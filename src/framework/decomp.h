#ifndef __DECOMP_H__
#define __DECOMP_H__

// Helpers for reconstructed layouts where the real member isn't known yet.
typedef unsigned char undefined;
typedef unsigned short undefined2;
typedef unsigned int undefined4;

// Usage: DECOMP_SIZE_ASSERT(WinDMApp, 0x738)
// The sizes are the 32-bit Windows layouts; they mean nothing on aarch64, so the checks are compiled out.
#define DECOMP_SIZE_ASSERT(type, size)

#endif // __DECOMP_H__
