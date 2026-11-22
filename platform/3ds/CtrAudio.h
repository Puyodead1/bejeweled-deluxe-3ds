#ifndef __CTRAUDIO_H__
#define __CTRAUDIO_H__

// Sound output through NDSP (the DSP): one stereo 16-bit channel at the DSP's own rate (32728 Hz, so the DSP doesn't
// resample), fed by the framework's mixer (CtrSoundManager: sound effects and the modplug music) from a thread that
// wakes on every DSP frame (~5 ms) and refills whichever buffers have played. Needs the DSP firmware dump at
// sdmc:/3ds/dspfirm.cdc; without it Open fails and the game runs silent.

#include <stdint.h>

namespace CtrAudio
{
	// Fills theFrames interleaved stereo frames; runs on the audio thread with the lock held.
	typedef void	(*MixFunc)(void* theUserData, int16_t* theOut, int theFrames);

	// Starts output and the thread that calls theMix. False if NDSP isn't available.
	bool		Open(MixFunc theMix, void* theUserData);
	void		Close();
	bool		IsOpen();
	int			GetRate();

	// Held while theMix runs; hold it to change anything theMix reads. Recursive.
	void		Lock();
	void		Unlock();
}

#endif
