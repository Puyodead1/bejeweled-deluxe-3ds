#ifndef __SWITCHAUDIO_H__
#define __SWITCHAUDIO_H__

// Sound output on the audio renderer (audren), in place of SDL's Switch audio driver. SDL keeps one buffer playing
// and one waiting, each as long as its callback (1024 frames, 21 ms, was the least Eden could keep fed), so a sound
// could wait ~43 ms to be heard. Here the buffers are one audren frame (5 ms) long and we keep only a few queued:
// three to start with, one more each time the queue runs dry, so the delay settles at what the system can keep up
// with (15 ms on a console that wakes the thread every frame, more in an emulator that doesn't).

#include <stdint.h>

namespace SwitchAudio
{
	// Fills theFrames interleaved stereo frames; runs on the audio thread with the lock held.
	typedef void	(*MixFunc)(void* theUserData, int16_t* theOut, int theFrames);

	// Starts 48 kHz stereo output and the thread that calls theMix. False if the audio renderer isn't available.
	bool		Open(MixFunc theMix, void* theUserData);
	void		Close();
	bool		IsOpen();
	int			GetRate();

	// Held while theMix runs; hold it to change anything theMix reads. Recursive.
	void		Lock();
	void		Unlock();
}

#endif
