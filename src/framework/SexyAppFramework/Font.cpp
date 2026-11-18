#include "Font.h"
#include "Image.h"

using namespace Sexy;

// FUNCTION: GAME187 0x425c80
Font::Font()
{	
	mAscent = 0;
	mHeight = 0;
	mAscentPadding = 0;
	mLineSpacingOffset = 0;
}

// FUNCTION: GAME187 0x425cc0
Font::Font(const Font& theFont) :
	mAscent(theFont.mAscent),
	mHeight(theFont.mHeight),
	mAscentPadding(theFont.mAscentPadding),
	mLineSpacingOffset(theFont.mLineSpacingOffset)
{
}

// FUNCTION: GAME187 0x425ce0
Font::~Font()
{
}

// FUNCTION: GAME187 0x425cf0
int	Font::GetAscent()
{
	return mAscent;
}

// FUNCTION: GAME187 0x425d00
int	Font::GetAscentPadding()
{
	return mAscentPadding;
}

// FUNCTION: GAME187 0x425d10
int	Font::GetDescent()
{
	return mHeight - mAscent;
}

// FUNCTION: GAME187 0x425d20
int	Font::GetHeight()
{
	return mHeight;
}

// FUNCTION: GAME187 0x425d30
int Font::GetLineSpacingOffset()
{
	return mLineSpacingOffset;
}

// FUNCTION: GAME187 0x425d40
int Font::GetLineSpacing()
{
	return mHeight + mLineSpacingOffset;
}

// FUNCTION: GAME187 0x425d50
int Font::StringWidth(const SexyString& theString)
{
	return 0;
}

// FUNCTION: GAME187 0x425d60
int Font::CharWidth(SexyChar theChar)
{
	SexyString aString(1, theChar);
	return StringWidth(aString);
}

// FUNCTION: GAME187 0x425e00
int Font::CharWidthKern(SexyChar theChar, SexyChar thePrevChar)
{
	return CharWidth(theChar);
}

void Font::DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect)
{
}
