#ifndef __DECOMP_H__
#define __DECOMP_H__

// Helpers for reconstructed layouts where the real member isn't known yet.
typedef unsigned char undefined;
typedef unsigned short undefined2;
typedef unsigned int undefined4;

// Compile-time size check. Usage: DECOMP_SIZE_ASSERT(WinDMApp, 0x738)
#define DECOMP_STATIC_ASSERT(cond) static void __decomp_static_assert_##__LINE__(int x[(cond) ? 1 : -1])
#define DECOMP_SIZE_ASSERT(type, size) DECOMP_STATIC_ASSERT(sizeof(type) == size)

#endif // __DECOMP_H__
