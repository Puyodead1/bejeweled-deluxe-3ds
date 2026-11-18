#include "SysFont.h"
#include "SexyAppBase.h"
#include "Graphics.h"

using namespace Sexy;

// FUNCTION: GAME187 0x436fd0
SysFont::SysFont(const std::string& theFace, int thePointSize, bool bold, bool italics, bool underline)
{
	Init(gSexyAppBase, theFace, thePointSize, ANSI_CHARSET, bold, italics, underline, false);
}

// FUNCTION: GAME187 0x437030
SysFont::SysFont(SexyAppBase* theApp, const std::string& theFace, int thePointSize, int theScript, bool bold, bool italics, bool underline)
{
	Init(theApp, theFace, thePointSize, theScript, bold, italics, underline, true);
}

// FUNCTION: GAME187 0x437060
void SysFont::Init(SexyAppBase* theApp, const std::string& theFace, int thePointSize, int theScript, bool bold, bool italics, bool underline, bool useDevCaps)
{
	mApp = theApp;
	mPointSize = thePointSize;
	mBold = bold;

	// GDI at 96 dpi: pixel height = points * 96 / 72, ascent about 80% of that.
	mHeight = (thePointSize * 96 + 71) / 72;
	mAscent = mHeight * 4 / 5;

	mDrawShadow = false;
	mSimulateBold = false;
}

// FUNCTION: GAME187 0x437140
SysFont::SysFont(const SysFont& theSysFont) : Font(theSysFont)
{
	mApp = theSysFont.mApp;
	mPointSize = theSysFont.mPointSize;
	mBold = theSysFont.mBold;
	mHeight = theSysFont.mHeight;
	mAscent = theSysFont.mAscent;

	mDrawShadow = false;
	mSimulateBold = false;
}

// FUNCTION: GAME187 0x4371b0
SysFont::~SysFont()
{
}

ImageFont* SysFont::CreateImageFont()
{
	return NULL;
}

int SysFont::StringWidth(const SexyString& theString)
{
	// Average Arial/Tahoma advance is a bit over half the pixel height.
	return (int)theString.length() * (mHeight * 11 / 20 + (mBold ? 1 : 0));
}

void SysFont::DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect)
{
	// TODO(switch): no system fonts; nothing in Bejeweled draws with a SysFont once its widgets have fonts set.
}

Font* SysFont::Duplicate()
{
	return new SysFont(*this);
}
