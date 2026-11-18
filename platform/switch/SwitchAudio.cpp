#include "SwitchAudio.h"

#include <switch.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

// One audren frame at 48 kHz: the renderer consumes a buffer per frame, so this is the finest the queue can be.
static const int FRAMES_PER_BUFFER = 240;
static const int NUM_BUFFERS = 16;			// the most we'd ever queue: 80 ms
static const int START_QUEUE = 3;
static const int RATE = 48000;

static const AudioRendererConfig gConfig =
{
	.output_rate		= AudioRendererOutputRate_48kHz,
	.num_voices			= 24,		// only voice 0 is used, but Eden never plays from a 1-voice renderer (SDL asks for 24)
	.num_effects		= 0,
	.num_sinks			= 1,
	.num_mix_objs		= 1,
	.num_mix_buffers	= 2,
};

static bool gRendererUp = false;
static bool gDriverUp = false;
static AudioDriver gDriver;
static int16_t* gPool = NULL;
static AudioDriverWaveBuf gBuffers[NUM_BUFFERS];
static Thread gThread;
static bool gThreadStarted = false;
static volatile bool gRunning = false;
static RMutex gLock;

static SwitchAudio::MixFunc gMix = NULL;
static void* gMixUserData = NULL;

static bool IsInFlight(const AudioDriverWaveBuf& theBuffer)
{
	return theBuffer.state == AudioDriverWaveBufState_Waiting || theBuffer.state == AudioDriverWaveBufState_Queued ||
		theBuffer.state == AudioDriverWaveBufState_Playing;
}

static void AudioThread(void* theArg)
{
	int aQueue = START_QUEUE;
	int aNext = 0;				// buffers play in order, so they're filled round-robin
	bool aStarted = false;
	uint64_t aLastWakeNs = armTicksToNs(armGetSystemTick());

	while (gRunning)
	{
		audrvUpdate(&gDriver);

		uint64_t aWakeNs = armTicksToNs(armGetSystemTick());
		uint64_t aGapNs = aWakeNs - aLastWakeNs;
		aLastWakeNs = aWakeNs;

		int anInFlight = 0;
		for (int i = 0; i < NUM_BUFFERS; i++)
			if (IsInFlight(gBuffers[i]))
				anInFlight++;

		// Everything we queued has played: we woke up too late. Keep one more buffer from now on. Not after a gap no
		// queue could cover (suspended for the HOME menu or sleep), or each trip to the HOME menu would add one.
		bool isCoverable = aGapNs < (uint64_t)NUM_BUFFERS * FRAMES_PER_BUFFER * 1000000000ULL / RATE;
		if (aStarted && anInFlight == 0 && isCoverable && aQueue < NUM_BUFFERS)
		{
			aQueue++;
			fprintf(stderr, "audio: ran dry, queueing %d buffers (%d ms) from now on\n", aQueue, aQueue * FRAMES_PER_BUFFER * 1000 / RATE);
		}

		while (anInFlight < aQueue && !IsInFlight(gBuffers[aNext]))
		{
			int16_t* anOut = gPool + aNext * FRAMES_PER_BUFFER * 2;
			rmutexLock(&gLock);
			if (gMix != NULL)
				gMix(gMixUserData, anOut, FRAMES_PER_BUFFER);
			else
				memset(anOut, 0, FRAMES_PER_BUFFER * 4);
			rmutexUnlock(&gLock);
			armDCacheFlush(anOut, FRAMES_PER_BUFFER * 4);

			audrvVoiceAddWaveBuf(&gDriver, 0, &gBuffers[aNext]);
			aNext = (aNext + 1) % NUM_BUFFERS;
			anInFlight++;
		}

		if (!audrvVoiceIsPlaying(&gDriver, 0))
			audrvVoiceStart(&gDriver, 0);
		audrvUpdate(&gDriver);
		aStarted = true;

		audrenWaitFrame();
	}
}

bool SwitchAudio::Open(MixFunc theMix, void* theUserData)
{
	if (gRunning)
		return true;

	rmutexInit(&gLock);
	gMix = theMix;
	gMixUserData = theUserData;

	Result aResult = audrenInitialize(&gConfig);
	if (R_FAILED(aResult))
	{
		fprintf(stderr, "audrenInitialize failed: 0x%x\n", (unsigned)aResult);
		Close();
		return false;
	}
	gRendererUp = true;

	aResult = audrvCreate(&gDriver, &gConfig, 2);
	if (R_FAILED(aResult))
	{
		fprintf(stderr, "audrvCreate failed: 0x%x\n", (unsigned)aResult);
		Close();
		return false;
	}
	gDriverUp = true;

	size_t aPoolSize = ((size_t)NUM_BUFFERS * FRAMES_PER_BUFFER * 4 + 0xFFF) & ~(size_t)0xFFF;
	gPool = (int16_t*)memalign(0x1000, aPoolSize);
	if (gPool == NULL)
	{
		Close();
		return false;
	}
	memset(gPool, 0, aPoolSize);
	armDCacheFlush(gPool, aPoolSize);

	// All buffers point into one memory pool; offsets are in frames.
	memset(gBuffers, 0, sizeof(gBuffers));
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
		gBuffers[i].data_raw = gPool;
		gBuffers[i].size = aPoolSize;
		gBuffers[i].start_sample_offset = i * FRAMES_PER_BUFFER;
		gBuffers[i].end_sample_offset = (i + 1) * FRAMES_PER_BUFFER;
	}

	int aPoolId = audrvMemPoolAdd(&gDriver, gPool, aPoolSize);
	audrvMemPoolAttach(&gDriver, aPoolId);

	static const u8 aSinkChannels[] = { 0, 1 };
	audrvDeviceSinkAdd(&gDriver, AUDREN_DEFAULT_DEVICE_NAME, 2, aSinkChannels);

	aResult = audrenStartAudioRenderer();
	if (R_FAILED(aResult))
	{
		fprintf(stderr, "audrenStartAudioRenderer failed: 0x%x\n", (unsigned)aResult);
		Close();
		return false;
	}

	audrvVoiceInit(&gDriver, 0, 2, PcmFormat_Int16, RATE);
	audrvVoiceSetDestinationMix(&gDriver, 0, AUDREN_FINAL_MIX_ID);
	audrvVoiceSetMixFactor(&gDriver, 0, 1.0f, 0, 0);
	audrvVoiceSetMixFactor(&gDriver, 0, 0.0f, 0, 1);
	audrvVoiceSetMixFactor(&gDriver, 0, 0.0f, 1, 0);
	audrvVoiceSetMixFactor(&gDriver, 0, 1.0f, 1, 1);
	audrvVoiceStart(&gDriver, 0);
	audrvUpdate(&gDriver);

	// Its own core if there's one free (the main thread is on 0, the loading thread on 1), above the main thread's
	// priority so a long frame can't starve it.
	s32 aPriority = 0x2C;
	svcGetThreadPriority(&aPriority, CUR_THREAD_HANDLE);
	aPriority = (aPriority > 0x18) ? aPriority - 1 : aPriority;
	gRunning = true;
	aResult = threadCreate(&gThread, AudioThread, NULL, NULL, 0x10000, aPriority, 2);
	if (R_FAILED(aResult))
		aResult = threadCreate(&gThread, AudioThread, NULL, NULL, 0x10000, aPriority, -2);
	if (R_SUCCEEDED(aResult))
		aResult = threadStart(&gThread);
	if (R_FAILED(aResult))
	{
		fprintf(stderr, "audio thread failed: 0x%x\n", (unsigned)aResult);
		gRunning = false;
		Close();
		return false;
	}
	gThreadStarted = true;

	fprintf(stderr, "audio: audren %d Hz, %d-frame buffers, %d queued (%d ms)\n", RATE, FRAMES_PER_BUFFER, START_QUEUE,
		START_QUEUE * FRAMES_PER_BUFFER * 1000 / RATE);
	return true;
}

void SwitchAudio::Close()
{
	gRunning = false;
	if (gThreadStarted)
	{
		threadWaitForExit(&gThread);
		threadClose(&gThread);
		gThreadStarted = false;
	}

	if (gDriverUp)
	{
		audrvVoiceStop(&gDriver, 0);
		audrvClose(&gDriver);
		gDriverUp = false;
	}
	if (gRendererUp)
	{
		audrenExit();
		gRendererUp = false;
	}
	if (gPool != NULL)
	{
		free(gPool);
		gPool = NULL;
	}
	gMix = NULL;
	gMixUserData = NULL;
}

bool SwitchAudio::IsOpen()
{
	return gRunning;
}

int SwitchAudio::GetRate()
{
	return RATE;
}

void SwitchAudio::Lock()
{
	rmutexLock(&gLock);
}

void SwitchAudio::Unlock()
{
	rmutexUnlock(&gLock);
}
