#include "CtrAudio.h"

#include <3ds.h>
#include <stdio.h>
#include <string.h>

// The DSP's output rate (the codec runs at 268 MHz / 8192 = 32728.5 Hz). Mixing at it means one resampling step (ours,
// of the 44.1 kHz sounds and the music) instead of two, and modplug renders 26% fewer samples than at 44.1 kHz.
static const float DSP_RATE = 32728.498f;
static const int RATE = 32728;
static const int CHANNEL = 0;
static const int FRAMES_PER_BUFFER = 512;		// 15.6 ms
static const int NUM_BUFFERS = 4;				// at most 62 ms queued

static bool gNdspUp = false;
static int16_t* gPool = NULL;
static ndspWaveBuf gBuffers[NUM_BUFFERS];
static Thread gThread = NULL;
static volatile bool gRunning = false;
static LightEvent gWake;
static RecursiveLock gLock;
static int gUnderruns = 0;

static CtrAudio::MixFunc gMix = NULL;
static void* gMixUserData = NULL;

// NDSP's thread, once per DSP frame.
static void DspCallback(void* theData)
{
	LightEvent_Signal(&gWake);
}

static void FillBuffer(ndspWaveBuf& theBuffer)
{
	int16_t* anOut = theBuffer.data_pcm16;
	RecursiveLock_Lock(&gLock);
	if (gMix != NULL)
		gMix(gMixUserData, anOut, FRAMES_PER_BUFFER);
	else
		memset(anOut, 0, FRAMES_PER_BUFFER * 4);
	RecursiveLock_Unlock(&gLock);
	DSP_FlushDataCache(anOut, FRAMES_PER_BUFFER * 4);
	ndspChnWaveBufAdd(CHANNEL, &theBuffer);
}

static void AudioThread(void* theArg)
{
	// Buffers play in the order they were queued, so they're refilled round-robin.
	int aNext = 0;
	while (gRunning)
	{
		int aFilled = 0;
		while (aFilled < NUM_BUFFERS)
		{
			ndspWaveBuf& aBuffer = gBuffers[aNext];
			if (aBuffer.status != NDSP_WBUF_FREE && aBuffer.status != NDSP_WBUF_DONE)
				break;
			FillBuffer(aBuffer);
			aNext = (aNext + 1) % NUM_BUFFERS;
			aFilled++;
		}
		if (aFilled == NUM_BUFFERS && gRunning)
		{
			// Everything had played: we were late (or just starting, or back from the HOME menu).
			gUnderruns++;
		}

		LightEvent_Wait(&gWake);
	}
}

bool CtrAudio::Open(MixFunc theMix, void* theUserData)
{
	if (gRunning)
		return true;

	RecursiveLock_Init(&gLock);
	LightEvent_Init(&gWake, RESET_ONESHOT);
	gMix = theMix;
	gMixUserData = theUserData;

	Result aResult = ndspInit();
	if (R_FAILED(aResult))
	{
		fprintf(stderr, "audio: ndspInit failed (0x%08lX); no DSP firmware at sdmc:/3ds/dspfirm.cdc? Running silent.\n", aResult);
		Close();
		return false;
	}
	gNdspUp = true;

	gPool = (int16_t*)linearAlloc(NUM_BUFFERS * FRAMES_PER_BUFFER * 4);
	if (gPool == NULL)
	{
		Close();
		return false;
	}
	memset(gPool, 0, NUM_BUFFERS * FRAMES_PER_BUFFER * 4);

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnReset(CHANNEL);
	ndspChnSetFormat(CHANNEL, NDSP_FORMAT_STEREO_PCM16);
	ndspChnSetRate(CHANNEL, DSP_RATE);
	ndspChnSetInterp(CHANNEL, NDSP_INTERP_NONE);
	float aMix[12] = { 1.0f, 1.0f };
	ndspChnSetMix(CHANNEL, aMix);

	memset(gBuffers, 0, sizeof(gBuffers));
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
		gBuffers[i].data_pcm16 = gPool + i * FRAMES_PER_BUFFER * 2;
		gBuffers[i].nsamples = FRAMES_PER_BUFFER;
		gBuffers[i].status = NDSP_WBUF_FREE;
	}

	// Above the main thread's priority on its core, so a long frame can't starve the DSP.
	s32 aPriority = 0x30;
	svcGetThreadPriority(&aPriority, CUR_THREAD_HANDLE);
	aPriority = (aPriority > 0x19) ? aPriority - 8 : 0x18;
	gRunning = true;
	gThread = threadCreate(AudioThread, NULL, 0x8000, aPriority, -2, false);
	if (gThread == NULL)
	{
		fprintf(stderr, "audio: thread failed\n");
		gRunning = false;
		Close();
		return false;
	}
	ndspSetCallback(DspCallback, NULL);

	fprintf(stderr, "audio: NDSP %d Hz, %d-frame buffers, %d of them (%d ms)\n", RATE, FRAMES_PER_BUFFER, NUM_BUFFERS,
		NUM_BUFFERS * FRAMES_PER_BUFFER * 1000 / RATE);
	return true;
}

void CtrAudio::Close()
{
	if (gThread != NULL)
	{
		gRunning = false;
		LightEvent_Signal(&gWake);
		threadJoin(gThread, U64_MAX);
		threadFree(gThread);
		gThread = NULL;
		fprintf(stderr, "audio: %d underruns\n", gUnderruns - 1);	// the first fill counts as one
	}
	gRunning = false;

	if (gNdspUp)
	{
		ndspSetCallback(NULL, NULL);
		ndspChnWaveBufClear(CHANNEL);
		ndspExit();
		gNdspUp = false;
	}
	if (gPool != NULL)
	{
		linearFree(gPool);
		gPool = NULL;
	}
	gMix = NULL;
	gMixUserData = NULL;
}

bool CtrAudio::IsOpen()
{
	return gRunning;
}

int CtrAudio::GetRate()
{
	return RATE;
}

void CtrAudio::Lock()
{
	RecursiveLock_Lock(&gLock);
}

void CtrAudio::Unlock()
{
	RecursiveLock_Unlock(&gLock);
}
