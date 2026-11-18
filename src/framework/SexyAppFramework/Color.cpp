#include "Color.h"

using namespace Sexy;

Color Color::Black(0, 0, 0);
Color Color::White(255, 255, 255);

// FUNCTION: GAME187 0x4042b0
Color::Color() :
	mRed(0),
	mGreen(0),
	mBlue(0),
	mAlpha(255)
{
}

// FUNCTION: GAME187 0x4042d0
Color::Color(int theColor) :
	mAlpha((theColor >> 24) & 0xFF),
	mRed((theColor   >> 16) & 0xFF),
	mGreen((theColor >> 8 ) & 0xFF),
	mBlue((theColor       ) & 0xFF)
{
	if(mAlpha==0)
		mAlpha = 0xff;
}

Color::Color(int theColor, int theAlpha) :
	mRed((theColor   >> 16) & 0xFF),
	mGreen((theColor >> 8 ) & 0xFF),
	mBlue((theColor       ) & 0xFF),
	mAlpha(theAlpha)
{
}

// FUNCTION: GAME187 0x404310
Color::Color(int theRed, int theGreen, int theBlue) :
	mRed(theRed),
	mGreen(theGreen),
	mBlue(theBlue),
	mAlpha(0xFF)
{
}

// FUNCTION: GAME187 0x483700
Color::Color(int theRed, int theGreen, int theBlue, int theAlpha) :
	mRed(theRed),
	mGreen(theGreen),
	mBlue(theBlue),
	mAlpha(theAlpha)
{
}

Color::Color(const SexyRGBA &theColor) :
	mRed(theColor.r),
	mGreen(theColor.g),
	mBlue(theColor.b),
	mAlpha(theColor.a)
{
}

Color::Color(const uchar* theElements) :
	mRed(theElements[0]),
	mGreen(theElements[1]),
	mBlue(theElements[2]),
	mAlpha(0xFF)
{
}

// FUNCTION: GAME187 0x404330
Color::Color(const int* theElements) :
	mRed(theElements[0]),
	mGreen(theElements[1]),
	mBlue(theElements[2]),
	mAlpha(0xFF)
{
}

// FUNCTION: GAME187 0x404350
int	Color::GetRed() const
{
	return mRed;	
}

int Color::GetGreen() const
{
	return mGreen;
}

int	Color::GetBlue() const
{
	return mBlue;
}

int	Color::GetAlpha() const
{
	return mAlpha;
}

int& Color::operator[](int theIdx)
{
	static int aJunk = 0;

	switch (theIdx)
	{
	case 0:
		return mRed;
	case 1:
		return mGreen;
	case 2:
		return mBlue;
	case 3:
		return mAlpha;
	default:
		return aJunk;
	}
}

int Color::operator[](int theIdx) const
{
	switch (theIdx)
	{
	case 0:
		return mRed;
	case 1:
		return mGreen;
	case 2:
		return mBlue;
	case 3:
		return mAlpha;
	default:
		return 0;
	}
}

// FUNCTION: GAME187 0x404370
uint32_t Color::ToInt() const
{
	return (mAlpha << 24) | (mRed << 16) | (mGreen << 8) | (mBlue);
}

SexyRGBA Color::ToRGBA() const
{
	SexyRGBA anRGBA;
	anRGBA.r = mRed;
	anRGBA.g = mGreen;
	anRGBA.b = mBlue;
	anRGBA.a = mAlpha;

	return anRGBA;
}

// FUNCTION: GAME187 0x404390
bool Sexy::operator==(const Color& theColor1, const Color& theColor2)
{
	return 
		(theColor1.mRed == theColor2.mRed) &&
		(theColor1.mGreen == theColor2.mGreen) &&
		(theColor1.mBlue == theColor2.mBlue) && 
		(theColor1.mAlpha == theColor2.mAlpha);
}

// FUNCTION: GAME187 0x4043d0
bool Sexy::operator!=(const Color& theColor1, const Color& theColor2)
{
	return 
		(theColor1.mRed != theColor2.mRed) ||
		(theColor1.mGreen != theColor2.mGreen) ||
		(theColor1.mBlue != theColor2.mBlue) ||
		(theColor1.mAlpha != theColor2.mAlpha);
}
