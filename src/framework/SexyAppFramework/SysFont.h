#ifndef __SYSFONT_H__
#define __SYSFONT_H__

#include "Font.h"

namespace Sexy
{

class ImageFont;
class SexyAppBase;

#define ANSI_CHARSET 0

// Switch port: the Windows version rendered through GDI. There are no system fonts here, so a SysFont only provides
// plausible metrics for the point size; widgets the game uses get ImageFonts assigned before they draw.
// VTABLE: GAME187 0x50cc84
class SysFont : public Font
{
public:
	SexyAppBase*			mApp;
	int						mPointSize;
	bool					mBold;
	bool					mDrawShadow;
	bool					mSimulateBold;

	void Init(SexyAppBase* theApp, const std::string& theFace, int thePointSize, int theScript, bool bold, bool italics, bool underline, bool useDevCaps);

public:
	SysFont(const std::string& theFace, int thePointSize, bool bold = false, bool italics = false, bool underline = false);
	SysFont(SexyAppBase* theApp, const std::string& theFace, int thePointSize, int theScript = ANSI_CHARSET, bool bold = false, bool italics = false, bool underline = false);
	SysFont(const SysFont& theSysFont);

	virtual ~SysFont();

	ImageFont*				CreateImageFont();
	virtual int				StringWidth(const SexyString& theString);
	virtual void			DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect);

	virtual Font*			Duplicate();
};

}

#endif //__SYSFONT_H__
