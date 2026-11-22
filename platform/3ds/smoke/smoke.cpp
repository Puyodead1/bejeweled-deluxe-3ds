// Phase 1 smoke test for the 3DS port: checks every system service the game will need, using the game's own data.
//
// - citro3d: draws a 640x480 "game frame" into an off-screen render target (a 1024x512 texture), then presents it
//   to both screens: the bottom screen gets the whole frame at half size (exactly 320x240), the top screen either the
//   same (pillarboxed) or a 1:1 window that follows the pointer (the planned "magnifier"; L/R toggles).
// - Textures: titlescreen6.jpg through libjpeg-turbo, gemsheet6.png through libpng, swizzled on the CPU.
// - RomFS: lists the packed data. SD card: writes sdmc:/3ds/Bejeweled/smoke.log.
// - NDSP: streams 1984-network.s3m through libmodplug on channel 0; A or a tap plays select.ogg (Tremor) on channel 1.
//   Without the DSP firmware (sdmc:/3ds/dspfirm.cdc) the test runs silent and says so in the log.
// - Input: touch (red marker, at 2x in game coordinates), Circle Pad / D-pad move a white pointer, buttons are logged.
//   START quits.
// A perf line goes to the log every 5 s: frame rate, CPU/GPU time per frame, music decode cost, free memory.

#include <3ds.h>
#include <citro3d.h>

#include <dirent.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <string>
#include <vector>

#include <jpeglib.h>
#include <png.h>
#include <tremor/ivorbisfile.h>
#include <libmodplug/modplug.h>

#include "sprite_shader_shbin.h"

namespace
{

// --- Logging -------------------------------------------------------------------------------------------------

FILE* gLog = NULL;

void Log(const char* theFormat, ...)
{
	va_list anArgs;
	va_start(anArgs, theFormat);
	vprintf(theFormat, anArgs);
	va_end(anArgs);
	putchar('\n');

	if (gLog != NULL)
	{
		va_start(anArgs, theFormat);
		vfprintf(gLog, theFormat, anArgs);
		va_end(anArgs);
		fputc('\n', gLog);
		fflush(gLog);
	}
}

uint64_t TicksToUs(uint64_t theTicks)
{
	return theTicks * 1000000ULL / SYSCLOCK_ARM11;
}

// --- Images ------------------------------------------------------------------------------------------------

// 32-bit A8R8G8B8 pixels (0xAARRGGBB), the framework's MemoryImage format.
struct Pixmap
{
	int						mWidth = 0;
	int						mHeight = 0;
	std::vector<uint32_t>	mBits;
};

bool ReadFile(const char* thePath, std::vector<uint8_t>& theData)
{
	FILE* aFile = fopen(thePath, "rb");
	if (aFile == NULL)
		return false;
	fseek(aFile, 0, SEEK_END);
	long aSize = ftell(aFile);
	fseek(aFile, 0, SEEK_SET);
	theData.resize(aSize);
	bool ok = aSize == 0 || fread(theData.data(), 1, aSize, aFile) == (size_t)aSize;
	fclose(aFile);
	return ok;
}

struct JpegError
{
	jpeg_error_mgr			mMgr;
	jmp_buf					mJump;
};

void JpegErrorExit(j_common_ptr theInfo)
{
	char aMessage[JMSG_LENGTH_MAX];
	theInfo->err->format_message(theInfo, aMessage);
	Log("jpeg: %s", aMessage);
	longjmp(((JpegError*)theInfo->err)->mJump, 1);
}

bool LoadJpeg(const char* thePath, Pixmap& thePixmap)
{
	std::vector<uint8_t> aData;
	if (!ReadFile(thePath, aData))
		return false;

	jpeg_decompress_struct anInfo;
	JpegError anError;
	anInfo.err = jpeg_std_error(&anError.mMgr);
	anError.mMgr.error_exit = JpegErrorExit;
	if (setjmp(anError.mJump))
	{
		jpeg_destroy_decompress(&anInfo);
		return false;
	}

	jpeg_create_decompress(&anInfo);
	jpeg_mem_src(&anInfo, aData.data(), aData.size());
	jpeg_read_header(&anInfo, TRUE);
	anInfo.out_color_space = JCS_EXT_BGRA;	// bytes B, G, R, A = 0xAARRGGBB little-endian
	jpeg_start_decompress(&anInfo);

	thePixmap.mWidth = anInfo.output_width;
	thePixmap.mHeight = anInfo.output_height;
	thePixmap.mBits.resize(thePixmap.mWidth * thePixmap.mHeight);
	while (anInfo.output_scanline < anInfo.output_height)
	{
		JSAMPROW aRow = (JSAMPROW)&thePixmap.mBits[anInfo.output_scanline * thePixmap.mWidth];
		jpeg_read_scanlines(&anInfo, &aRow, 1);
	}

	jpeg_finish_decompress(&anInfo);
	jpeg_destroy_decompress(&anInfo);
	return true;
}

bool LoadPng(const char* thePath, Pixmap& thePixmap)
{
	png_image anImage;
	memset(&anImage, 0, sizeof(anImage));
	anImage.version = PNG_IMAGE_VERSION;
	if (!png_image_begin_read_from_file(&anImage, thePath))
	{
		Log("png: %s: %s", thePath, anImage.message);
		return false;
	}

	anImage.format = PNG_FORMAT_BGRA;	// 0xAARRGGBB little-endian
	thePixmap.mWidth = anImage.width;
	thePixmap.mHeight = anImage.height;
	thePixmap.mBits.resize(anImage.width * anImage.height);
	if (!png_image_finish_read(&anImage, NULL, thePixmap.mBits.data(), 0, NULL))
	{
		Log("png: %s: %s", thePath, anImage.message);
		return false;
	}
	return true;
}

// --- Textures ----------------------------------------------------------------------------------------------

uint32_t NextPow2(uint32_t theValue)
{
	uint32_t aResult = 8;
	while (aResult < theValue)
		aResult <<= 1;
	return aResult;
}

// Offset of texel (x, y) in a PICA200 texture of the given width: 8x8 tiles stored row by row, Morton (Z) order
// inside a tile. Texture memory row 0 is t = 0, and the renderer puts image row 0 there, so a texture coordinate
// is simply (x / width, y / height) with y pointing down the image.
inline uint32_t TexelOffset(uint32_t x, uint32_t y, uint32_t theWidth)
{
	uint32_t aMorton = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
	return ((y >> 3) * (theWidth >> 3) + (x >> 3)) * 64 + aMorton;
}

struct Texture
{
	C3D_Tex					mTex;
	int						mWidth = 0;		// image size; the texture itself is the next power of two
	int						mHeight = 0;
	bool					mValid = false;
};

bool MakeTexture(Texture& theTexture, const Pixmap& thePixmap)
{
	uint32_t aTexWidth = NextPow2(thePixmap.mWidth);
	uint32_t aTexHeight = NextPow2(thePixmap.mHeight);
	if (aTexWidth > 1024 || aTexHeight > 1024)
	{
		Log("texture: %dx%d is too big for one texture", thePixmap.mWidth, thePixmap.mHeight);
		return false;
	}
	if (!C3D_TexInit(&theTexture.mTex, aTexWidth, aTexHeight, GPU_RGBA8))
	{
		Log("texture: C3D_TexInit %lux%lu failed", aTexWidth, aTexHeight);
		return false;
	}

	// GPU_RGBA8 texels are 0xRRGGBBAA; the image is 0xAARRGGBB.
	uint32_t* aDest = (uint32_t*)theTexture.mTex.data;
	memset(aDest, 0, aTexWidth * aTexHeight * 4);
	const uint32_t* aSrc = thePixmap.mBits.data();
	for (int y = 0; y < thePixmap.mHeight; y++)
	{
		for (int x = 0; x < thePixmap.mWidth; x++)
		{
			uint32_t aPixel = *aSrc++;
			aDest[TexelOffset(x, y, aTexWidth)] = (aPixel << 8) | (aPixel >> 24);
		}
	}
	C3D_TexFlush(&theTexture.mTex);
	C3D_TexSetFilter(&theTexture.mTex, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetWrap(&theTexture.mTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

	theTexture.mWidth = thePixmap.mWidth;
	theTexture.mHeight = thePixmap.mHeight;
	theTexture.mValid = true;
	return true;
}

// --- Drawing -----------------------------------------------------------------------------------------------

struct Vertex
{
	float					mX, mY, mZ;
	float					mU, mV;
	uint32_t				mColor;			// bytes R, G, B, A
};

const int MAX_VERTICES = 4096;
const int GAME_WIDTH = 640;
const int GAME_HEIGHT = 480;
const int TARGET_WIDTH = 1024;
const int TARGET_HEIGHT = 512;

DVLB_s*			gShaderDvlb = NULL;
shaderProgram_s	gProgram;
int				gProjectionLoc = -1;
Vertex*			gVertices = NULL;
int				gVertexCount = 0;

uint32_t Rgba(int r, int g, int b, int a = 255)
{
	return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

void InitRenderer()
{
	gShaderDvlb = DVLB_ParseFile((u32*)sprite_shader_shbin, sprite_shader_shbin_size);
	shaderProgramInit(&gProgram);
	shaderProgramSetVsh(&gProgram, &gShaderDvlb->DVLE[0]);
	C3D_BindProgram(&gProgram);
	gProjectionLoc = shaderInstanceGetUniformLocation(gProgram.vertexShader, "projection");

	C3D_AttrInfo* anAttrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(anAttrInfo);
	AttrInfo_AddLoader(anAttrInfo, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(anAttrInfo, 1, GPU_FLOAT, 2);
	AttrInfo_AddLoader(anAttrInfo, 2, GPU_UNSIGNED_BYTE, 4);

	gVertices = (Vertex*)linearAlloc(MAX_VERTICES * sizeof(Vertex));
	C3D_BufInfo* aBufInfo = C3D_GetBufInfo();
	BufInfo_Init(aBufInfo);
	BufInfo_Add(aBufInfo, gVertices, sizeof(Vertex), 3, 0x210);

	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
	C3D_CullFace(GPU_CULL_NONE);
}

void SetProjection(const C3D_Mtx& theProjection)
{
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, gProjectionLoc, &theProjection);
}

void SetBlend(bool additive)
{
	if (additive)
		C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE, GPU_SRC_ALPHA, GPU_ONE);
	else
		C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
}

void SetTexture(C3D_Tex* theTex)
{
	C3D_TexEnv* anEnv = C3D_GetTexEnv(0);
	C3D_TexEnvInit(anEnv);
	if (theTex != NULL)
	{
		C3D_TexBind(0, theTex);
		C3D_TexEnvSrc(anEnv, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
		C3D_TexEnvFunc(anEnv, C3D_Both, GPU_MODULATE);
	}
	else
	{
		C3D_TexEnvSrc(anEnv, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
		C3D_TexEnvFunc(anEnv, C3D_Both, GPU_REPLACE);
	}
}

// A quad from four corners (top-left, top-right, bottom-left, bottom-right) and its texture rectangle.
void DrawQuad(const float theCorners[4][2], float u0, float v0, float u1, float v1, uint32_t theColor)
{
	if (gVertexCount + 6 > MAX_VERTICES)
		return;

	const float aUV[4][2] = { { u0, v0 }, { u1, v0 }, { u0, v1 }, { u1, v1 } };
	static const int kOrder[6] = { 0, 2, 1, 1, 2, 3 };
	Vertex* aVertex = &gVertices[gVertexCount];
	for (int i = 0; i < 6; i++)
	{
		int c = kOrder[i];
		aVertex[i] = { theCorners[c][0], theCorners[c][1], 0.5f, aUV[c][0], aUV[c][1], theColor };
	}
	C3D_DrawArrays(GPU_TRIANGLES, gVertexCount, 6);
	gVertexCount += 6;
}

void DrawRect(float x, float y, float w, float h, float u0, float v0, float u1, float v1, uint32_t theColor)
{
	const float aCorners[4][2] = { { x, y }, { x + w, y }, { x, y + h }, { x + w, y + h } };
	DrawQuad(aCorners, u0, v0, u1, v1, theColor);
}

// Draws part of a texture (source rectangle in image pixels) at x, y, optionally rotated about its centre.
void DrawImage(const Texture& theTexture, float x, float y, int sx, int sy, int sw, int sh, uint32_t theColor, float theAngle = 0.0f)
{
	float aTexW = theTexture.mTex.width;
	float aTexH = theTexture.mTex.height;
	float u0 = sx / aTexW, v0 = sy / aTexH, u1 = (sx + sw) / aTexW, v1 = (sy + sh) / aTexH;
	SetTexture((C3D_Tex*)&theTexture.mTex);
	if (theAngle == 0.0f)
	{
		DrawRect(x, y, sw, sh, u0, v0, u1, v1, theColor);
		return;
	}

	float cx = x + sw * 0.5f, cy = y + sh * 0.5f;
	float c = cosf(theAngle), s = sinf(theAngle);
	const float aLocal[4][2] = { { -sw * 0.5f, -sh * 0.5f }, { sw * 0.5f, -sh * 0.5f }, { -sw * 0.5f, sh * 0.5f }, { sw * 0.5f, sh * 0.5f } };
	float aCorners[4][2];
	for (int i = 0; i < 4; i++)
	{
		aCorners[i][0] = cx + aLocal[i][0] * c - aLocal[i][1] * s;
		aCorners[i][1] = cy + aLocal[i][0] * s + aLocal[i][1] * c;
	}
	DrawQuad(aCorners, u0, v0, u1, v1, theColor);
}

void FillRect(float x, float y, float w, float h, uint32_t theColor)
{
	SetTexture(NULL);
	DrawRect(x, y, w, h, 0, 0, 0, 0, theColor);
}

// --- RomFS -------------------------------------------------------------------------------------------------

void ListDir(const std::string& thePath, int& theFiles, long long& theBytes)
{
	DIR* aDir = opendir(thePath.c_str());
	if (aDir == NULL)
		return;
	while (dirent* anEntry = readdir(aDir))
	{
		if (strcmp(anEntry->d_name, ".") == 0 || strcmp(anEntry->d_name, "..") == 0)
			continue;
		std::string aPath = thePath + "/" + anEntry->d_name;
		struct stat aStat;
		if (stat(aPath.c_str(), &aStat) != 0)
			continue;
		if (S_ISDIR(aStat.st_mode))
			ListDir(aPath, theFiles, theBytes);
		else
		{
			theFiles++;
			theBytes += aStat.st_size;
		}
	}
	closedir(aDir);
}

void LogRomFS()
{
	static const char* kFolders[] = { "data", "images", "music", "properties", "sounds" };
	int aTotalFiles = 0;
	for (const char* aFolder : kFolders)
	{
		int aFiles = 0;
		long long aBytes = 0;
		ListDir(std::string("romfs:/") + aFolder, aFiles, aBytes);
		Log("romfs: %-10s %3d files %8lld bytes", aFolder, aFiles, aBytes);
		aTotalFiles += aFiles;
	}
	Log("romfs: %d files in all", aTotalFiles);
}

// --- Audio -------------------------------------------------------------------------------------------------

const int MUSIC_CHANNEL = 0;
const int SFX_CHANNEL = 1;
const int MUSIC_RATE = 44100;
const int MUSIC_BUFFERS = 4;
const int MUSIC_FRAMES = 2048;	// per buffer: 46 ms at 44.1 kHz

bool			gAudio = false;
ModPlugFile*	gMusic = NULL;
ndspWaveBuf		gMusicBufs[MUSIC_BUFFERS];
uint64_t		gMusicDecodeTicks = 0;
uint64_t		gMusicDecodedFrames = 0;
int				gMusicUnderruns = 0;

ndspWaveBuf		gSfxBuf;
int16_t*		gSfxData = NULL;

void FillMusicBuffer(ndspWaveBuf& theBuf)
{
	uint64_t aStart = svcGetSystemTick();
	int aBytes = ModPlug_Read(gMusic, theBuf.data_pcm16, MUSIC_FRAMES * 2 * sizeof(int16_t));
	gMusicDecodeTicks += svcGetSystemTick() - aStart;
	if (aBytes <= 0)
		return;
	theBuf.nsamples = aBytes / (2 * sizeof(int16_t));
	gMusicDecodedFrames += theBuf.nsamples;
	DSP_FlushDataCache(theBuf.data_pcm16, aBytes);
	ndspChnWaveBufAdd(MUSIC_CHANNEL, &theBuf);
}

void PumpMusic()
{
	if (gMusic == NULL)
		return;
	int aDone = 0;
	for (ndspWaveBuf& aBuf : gMusicBufs)
	{
		if (aBuf.status == NDSP_WBUF_DONE)
		{
			FillMusicBuffer(aBuf);
			aDone++;
		}
	}
	if (aDone == MUSIC_BUFFERS)
		gMusicUnderruns++;
}

bool LoadMusic(const char* thePath)
{
	std::vector<uint8_t> aData;
	if (!ReadFile(thePath, aData))
	{
		Log("music: can't read %s", thePath);
		return false;
	}

	ModPlug_Settings aSettings;
	ModPlug_GetSettings(&aSettings);
	aSettings.mFlags = MODPLUG_ENABLE_OVERSAMPLING;
	aSettings.mChannels = 2;
	aSettings.mBits = 16;
	aSettings.mFrequency = MUSIC_RATE;
	aSettings.mResamplingMode = MODPLUG_RESAMPLE_LINEAR;
	aSettings.mLoopCount = -1;
	ModPlug_SetSettings(&aSettings);

	gMusic = ModPlug_Load(aData.data(), aData.size());
	if (gMusic == NULL)
	{
		Log("music: ModPlug_Load failed for %s", thePath);
		return false;
	}
	Log("music: %s \"%s\", %d s", thePath, ModPlug_GetName(gMusic), ModPlug_GetLength(gMusic) / 1000);

	ndspChnReset(MUSIC_CHANNEL);
	ndspChnSetFormat(MUSIC_CHANNEL, NDSP_FORMAT_STEREO_PCM16);
	ndspChnSetRate(MUSIC_CHANNEL, MUSIC_RATE);
	ndspChnSetInterp(MUSIC_CHANNEL, NDSP_INTERP_LINEAR);
	for (ndspWaveBuf& aBuf : gMusicBufs)
	{
		memset(&aBuf, 0, sizeof(aBuf));
		aBuf.data_pcm16 = (int16_t*)linearAlloc(MUSIC_FRAMES * 2 * sizeof(int16_t));
		aBuf.status = NDSP_WBUF_DONE;
	}
	for (ndspWaveBuf& aBuf : gMusicBufs)
		FillMusicBuffer(aBuf);
	return true;
}

bool LoadSound(const char* thePath)
{
	FILE* aFile = fopen(thePath, "rb");
	if (aFile == NULL)
	{
		Log("sound: can't open %s", thePath);
		return false;
	}

	OggVorbis_File aVorbis;
	if (ov_open(aFile, &aVorbis, NULL, 0) != 0)
	{
		Log("sound: ov_open failed for %s", thePath);
		fclose(aFile);
		return false;
	}

	vorbis_info* anInfo = ov_info(&aVorbis, -1);
	int aChannels = anInfo->channels;
	int aRate = anInfo->rate;
	std::vector<int16_t> aPcm;
	char aBuffer[4096];
	uint64_t aStart = svcGetSystemTick();
	for (;;)
	{
		long aRead = ov_read(&aVorbis, aBuffer, sizeof(aBuffer), NULL);
		if (aRead <= 0)
			break;
		size_t anOld = aPcm.size();
		aPcm.resize(anOld + aRead / 2);
		memcpy(&aPcm[anOld], aBuffer, aRead);
	}
	uint64_t aUs = TicksToUs(svcGetSystemTick() - aStart);
	ov_clear(&aVorbis);	// closes aFile

	int aFrames = aPcm.size() / aChannels;
	Log("sound: %s %d Hz, %d ch, %d frames, decoded in %llu us", thePath, aRate, aChannels, aFrames, aUs);
	if (aFrames == 0)
		return false;

	gSfxData = (int16_t*)linearAlloc(aPcm.size() * sizeof(int16_t));
	memcpy(gSfxData, aPcm.data(), aPcm.size() * sizeof(int16_t));
	DSP_FlushDataCache(gSfxData, aPcm.size() * sizeof(int16_t));

	ndspChnReset(SFX_CHANNEL);
	ndspChnSetFormat(SFX_CHANNEL, aChannels == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
	ndspChnSetRate(SFX_CHANNEL, aRate);
	ndspChnSetInterp(SFX_CHANNEL, NDSP_INTERP_LINEAR);
	memset(&gSfxBuf, 0, sizeof(gSfxBuf));
	gSfxBuf.data_pcm16 = gSfxData;
	gSfxBuf.nsamples = aFrames;
	return true;
}

void PlaySound()
{
	if (gSfxData == NULL)
		return;
	ndspChnWaveBufClear(SFX_CHANNEL);
	gSfxBuf.status = NDSP_WBUF_FREE;
	ndspChnWaveBufAdd(SFX_CHANNEL, &gSfxBuf);
}

// --- Input -------------------------------------------------------------------------------------------------

void LogKeys(u32 theKeys, const char* theWhat)
{
	static const struct { u32 mKey; const char* mName; } kKeys[] = {
		{ KEY_A, "A" }, { KEY_B, "B" }, { KEY_X, "X" }, { KEY_Y, "Y" }, { KEY_L, "L" }, { KEY_R, "R" },
		{ KEY_ZL, "ZL" }, { KEY_ZR, "ZR" }, { KEY_START, "START" }, { KEY_SELECT, "SELECT" },
		{ KEY_DUP, "UP" }, { KEY_DDOWN, "DOWN" }, { KEY_DLEFT, "LEFT" }, { KEY_DRIGHT, "RIGHT" },
	};
	for (const auto& aKey : kKeys)
	{
		if (theKeys & aKey.mKey)
			Log("input: %s %s", aKey.mName, theWhat);
	}
}

} // namespace

int main(int argc, char** argv)
{
	osSetSpeedupEnable(true);
	gfxInitDefault();
	romfsInit();

	mkdir("sdmc:/3ds", 0777);
	mkdir("sdmc:/3ds/Bejeweled", 0777);
	gLog = fopen("sdmc:/3ds/Bejeweled/smoke.log", "w");

	bool isNew3DS = false;
	APT_CheckNew3DS(&isNew3DS);
	Log("smoke: Bejeweled 3DS port, phase 1 smoke test (%s)", isNew3DS ? "New 3DS" : "Old 3DS");
	Log("memory: application %lu KB, free %lu KB, linear free %lu KB", osGetMemRegionSize(MEMREGION_APPLICATION) / 1024,
		osGetMemRegionFree(MEMREGION_APPLICATION) / 1024, linearSpaceFree() / 1024);

	LogRomFS();

	// Graphics
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	const u32 kDisplayFlags = GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
		GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);
	C3D_RenderTarget* aTop = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, -1);
	C3D_RenderTarget* aBottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, -1);
	C3D_RenderTargetSetOutput(aTop, GFX_TOP, GFX_LEFT, kDisplayFlags);
	C3D_RenderTargetSetOutput(aBottom, GFX_BOTTOM, GFX_LEFT, kDisplayFlags);

	// The game frame: a 1024x512 texture in VRAM, 640x480 of it used.
	C3D_Tex aFrameTex;
	bool haveFrame = C3D_TexInitVRAM(&aFrameTex, TARGET_WIDTH, TARGET_HEIGHT, GPU_RGBA8);
	C3D_RenderTarget* aFrameTarget = haveFrame ? C3D_RenderTargetCreateFromTex(&aFrameTex, GPU_TEXFACE_2D, 0, -1) : NULL;
	if (aFrameTarget == NULL)
		Log("video: can't create the 1024x512 frame target");
	C3D_TexSetFilter(&aFrameTex, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetWrap(&aFrameTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

	InitRenderer();

	// Pixel coordinates with y pointing down. In the frame target pixel row 0 is clip y = -1, which the GPU stores
	// at memory row 0, i.e. texture t = 0: the same convention as the textures we upload.
	C3D_Mtx aFrameProj, aTopProj, aBottomProj;
	Mtx_Ortho(&aFrameProj, 0.0f, TARGET_WIDTH, 0.0f, TARGET_HEIGHT, 1.0f, -1.0f, true);
	Mtx_OrthoTilt(&aTopProj, 0.0f, 400.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
	Mtx_OrthoTilt(&aBottomProj, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);

	// Images
	Texture aTitle, aGems;
	Pixmap aPixmap;
	uint64_t aStart = svcGetSystemTick();
	if (LoadJpeg("romfs:/images/titlescreen6.jpg", aPixmap))
	{
		Log("image: titlescreen6.jpg %dx%d, decoded in %llu us", aPixmap.mWidth, aPixmap.mHeight, TicksToUs(svcGetSystemTick() - aStart));
		MakeTexture(aTitle, aPixmap);
	}
	aStart = svcGetSystemTick();
	if (LoadPng("romfs:/images/gemsheet6.png", aPixmap))
	{
		Log("image: gemsheet6.png %dx%d, decoded in %llu us", aPixmap.mWidth, aPixmap.mHeight, TicksToUs(svcGetSystemTick() - aStart));
		MakeTexture(aGems, aPixmap);
	}
	aPixmap = Pixmap();
	Log("memory: after images, free %lu KB, linear free %lu KB", osGetMemRegionFree(MEMREGION_APPLICATION) / 1024, linearSpaceFree() / 1024);

	// Audio
	Result aResult = ndspInit();
	if (R_FAILED(aResult))
		Log("audio: ndspInit failed (0x%08lX): no DSP firmware at sdmc:/3ds/dspfirm.cdc? Running silent.", aResult);
	else
	{
		gAudio = true;
		ndspSetOutputMode(NDSP_OUTPUT_STEREO);
		LoadMusic("romfs:/music/1984-network.s3m");
		LoadSound("romfs:/sounds/select.ogg");
	}

	float aPointerX = GAME_WIDTH / 2, aPointerY = GAME_HEIGHT / 2;
	bool aMagnifier = false;
	bool aTouching = false;
	int aTouchX = 0, aTouchY = 0;
	int aFrame = 0;
	int aFramesThisPeriod = 0;
	float aCpuMs = 0.0f, aGpuMs = 0.0f;
	uint64_t aPeriodStart = svcGetSystemTick();

	while (aptMainLoop())
	{
		hidScanInput();
		u32 aDown = hidKeysDown();
		u32 aHeld = hidKeysHeld();
		u32 aUp = hidKeysUp();
		LogKeys(aDown, "down");
		if (aDown & KEY_START)
			break;
		if (aDown & (KEY_L | KEY_R))
		{
			aMagnifier = !aMagnifier;
			Log("video: top screen shows %s", aMagnifier ? "a 1:1 window at the pointer" : "the whole frame");
		}
		if (aDown & KEY_A)
			PlaySound();

		// Circle Pad glides the pointer, the d-pad nudges it.
		circlePosition aCircle;
		hidCircleRead(&aCircle);
		if (abs(aCircle.dx) > 15)
			aPointerX += aCircle.dx / 16.0f;
		if (abs(aCircle.dy) > 15)
			aPointerY -= aCircle.dy / 16.0f;
		if (aHeld & KEY_DLEFT) aPointerX -= 4;
		if (aHeld & KEY_DRIGHT) aPointerX += 4;
		if (aHeld & KEY_DUP) aPointerY -= 4;
		if (aHeld & KEY_DDOWN) aPointerY += 4;

		if (aHeld & KEY_TOUCH)
		{
			touchPosition aTouch;
			hidTouchRead(&aTouch);
			aTouchX = aTouch.px * 2;
			aTouchY = aTouch.py * 2;
			aPointerX = aTouchX;
			aPointerY = aTouchY;
			if (!aTouching)
			{
				Log("input: touch down at %d,%d (game %d,%d)", aTouch.px, aTouch.py, aTouchX, aTouchY);
				PlaySound();
			}
			aTouching = true;
		}
		else if (aTouching && (aUp & KEY_TOUCH))
		{
			Log("input: touch up at game %d,%d", aTouchX, aTouchY);
			aTouching = false;
		}
		aPointerX = fminf(fmaxf(aPointerX, 0.0f), GAME_WIDTH - 1);
		aPointerY = fminf(fmaxf(aPointerY, 0.0f), GAME_HEIGHT - 1);

		if (gAudio)
			PumpMusic();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		gVertexCount = 0;

		// 1. The game frame.
		if (aFrameTarget != NULL)
		{
			C3D_RenderTargetClear(aFrameTarget, C3D_CLEAR_COLOR, 0x000000FF, 0);
			C3D_FrameDrawOn(aFrameTarget);
			SetProjection(aFrameProj);
			SetBlend(false);

			if (aTitle.mValid)
				DrawImage(aTitle, 0, 0, 0, 0, aTitle.mWidth, aTitle.mHeight, Rgba(255, 255, 255));
			else
				FillRect(0, 0, GAME_WIDTH, GAME_HEIGHT, Rgba(40, 0, 80));

			// A row of 52x52 cells from the gem sheet, bobbing, and one spinning.
			if (aGems.mValid)
			{
				for (int i = 0; i < 7; i++)
				{
					float aBob = sinf((aFrame + i * 9) * 0.08f) * 8.0f;
					DrawImage(aGems, 140 + i * 52, 330 + aBob, i * 52, 0, 52, 52, Rgba(255, 255, 255));
				}
				DrawImage(aGems, 294, 400, 0, 52, 52, 52, Rgba(255, 255, 255), aFrame * 0.05f);
			}

			// Blending checks: a translucent bar and an additive glow that pulses.
			FillRect(20, 20, 200, 30, Rgba(0, 0, 0, 128));
			SetBlend(true);
			int aGlow = (int)(127 + 127 * sinf(aFrame * 0.1f));
			FillRect(30, 25, 180, 20, Rgba(255, 160, 0, aGlow));
			SetBlend(false);

			// Pointer and touch marker.
			FillRect(aPointerX - 6, aPointerY - 6, 12, 12, Rgba(255, 255, 255, 220));
			if (aTouching)
				FillRect(aTouchX - 4, aTouchY - 4, 8, 8, Rgba(255, 0, 0));
		}

		// 2. Present: the frame texture onto both screens.
		const float aFrameU = (float)GAME_WIDTH / TARGET_WIDTH;
		const float aFrameV = (float)GAME_HEIGHT / TARGET_HEIGHT;

		C3D_RenderTargetClear(aTop, C3D_CLEAR_COLOR, 0x000000FF, 0);
		C3D_FrameDrawOn(aTop);
		SetProjection(aTopProj);
		SetBlend(false);
		if (aFrameTarget != NULL)
		{
			SetTexture(&aFrameTex);
			if (aMagnifier)
			{
				// A 400x240 window into the frame at 1:1, centred on the pointer.
				float x0 = fminf(fmaxf(aPointerX - 200, 0.0f), GAME_WIDTH - 400);
				float y0 = fminf(fmaxf(aPointerY - 120, 0.0f), GAME_HEIGHT - 240);
				DrawRect(0, 0, 400, 240, x0 / TARGET_WIDTH, y0 / TARGET_HEIGHT, (x0 + 400) / TARGET_WIDTH, (y0 + 240) / TARGET_HEIGHT, Rgba(255, 255, 255));
			}
			else
				DrawRect(40, 0, 320, 240, 0, 0, aFrameU, aFrameV, Rgba(255, 255, 255));
		}

		C3D_RenderTargetClear(aBottom, C3D_CLEAR_COLOR, 0x000000FF, 0);
		C3D_FrameDrawOn(aBottom);
		SetProjection(aBottomProj);
		if (aFrameTarget != NULL)
		{
			SetTexture(&aFrameTex);
			DrawRect(0, 0, 320, 240, 0, 0, aFrameU, aFrameV, Rgba(255, 255, 255));
		}

		// The vertices were written through the CPU cache; the GPU reads memory.
		GSPGPU_FlushDataCache(gVertices, gVertexCount * sizeof(Vertex));
		C3D_FrameEnd(0);

		aFrame++;
		aFramesThisPeriod++;
		aCpuMs += C3D_GetProcessingTime();
		aGpuMs += C3D_GetDrawingTime();
		uint64_t aNow = svcGetSystemTick();
		uint64_t aPeriodUs = TicksToUs(aNow - aPeriodStart);
		if (aPeriodUs >= 5000000)
		{
			double aSeconds = aPeriodUs / 1e6;
			double aMusicMsPerSec = gMusicDecodedFrames > 0 ? TicksToUs(gMusicDecodeTicks) / 1000.0 / (gMusicDecodedFrames / (double)MUSIC_RATE) : 0.0;
			Log("perf: %.1f fps, cpu %.2f ms, gpu %.2f ms per frame; music decode %.1f ms per s of audio, %d underruns; free %lu KB, linear %lu KB",
				aFramesThisPeriod / aSeconds, aCpuMs / aFramesThisPeriod, aGpuMs / aFramesThisPeriod, aMusicMsPerSec, gMusicUnderruns,
				osGetMemRegionFree(MEMREGION_APPLICATION) / 1024, linearSpaceFree() / 1024);
			aFramesThisPeriod = 0;
			aCpuMs = aGpuMs = 0.0f;
			gMusicDecodeTicks = 0;
			gMusicDecodedFrames = 0;
			gMusicUnderruns = 0;
			aPeriodStart = aNow;
		}
	}

	Log("smoke: exiting after %d frames", aFrame);

	if (gAudio)
	{
		ndspChnWaveBufClear(MUSIC_CHANNEL);
		ndspChnWaveBufClear(SFX_CHANNEL);
		if (gMusic != NULL)
			ModPlug_Unload(gMusic);
		for (ndspWaveBuf& aBuf : gMusicBufs)
			if (aBuf.data_pcm16 != NULL)
				linearFree(aBuf.data_pcm16);
		if (gSfxData != NULL)
			linearFree(gSfxData);
		ndspExit();
	}

	if (aTitle.mValid) C3D_TexDelete(&aTitle.mTex);
	if (aGems.mValid) C3D_TexDelete(&aGems.mTex);
	if (aFrameTarget != NULL) C3D_RenderTargetDelete(aFrameTarget);
	if (haveFrame) C3D_TexDelete(&aFrameTex);
	C3D_RenderTargetDelete(aTop);
	C3D_RenderTargetDelete(aBottom);
	linearFree(gVertices);
	shaderProgramFree(&gProgram);
	DVLB_Free(gShaderDvlb);
	C3D_Fini();

	if (gLog != NULL)
		fclose(gLog);
	romfsExit();
	gfxExit();
	return 0;
}
