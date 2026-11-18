#include "Checkbox.h"
#include "CheckboxListener.h"
#include "Graphics.h"

using namespace Sexy;

// FUNCTION: GAME187 0x485c90
Checkbox::Checkbox(Image* theUncheckedImage, Image* theCheckedImage, int theId, CheckboxListener* theCheckboxListener) :	
	mUncheckedImage(theUncheckedImage),
	mCheckedImage(theCheckedImage),
	mId(theId),
	mListener(theCheckboxListener),
	mChecked(false)	,
	mOutlineColor(Color::White),
	mBkgColor(Color(80, 80, 80)),
	mCheckColor(Color(255, 255, 0))
{
	mDoFinger = true;
}

// FUNCTION: GAME187 0x485d80
void Checkbox::SetChecked(bool checked, bool tellListener)
{
	mChecked = checked;
	if (tellListener && mListener)
		mListener->CheckboxChecked(mId, mChecked);
	MarkDirty();
}

// FUNCTION: GAME187 0x485dc0
bool Checkbox::IsChecked()
{
	return mChecked;
}

// FUNCTION: GAME187 0x485dd0
void Checkbox::Draw(Graphics* g)
{
	Widget::Draw(g);

	/*if (mChecked)
		g->DrawImage(mApp->mImages[IMAGE_CHECKBOX], 0, 0, Rect(0, 0, 31, 32));
	else
		g->DrawImage(mApp->mImages[IMAGE_CHECKBOX], 0, 0, Rect(31, 0, 31, 32));*/
	
	if ((mCheckedRect.mWidth == 0) && (mCheckedImage != NULL) && (mUncheckedImage != NULL))
	{
		if (mChecked)
			g->DrawImage(mCheckedImage, 0, 0);
		else
			g->DrawImage(mUncheckedImage, 0, 0);
	}
	else if ((mCheckedRect.mWidth != 0) && (mUncheckedImage != NULL))
	{
		if (mChecked)
			g->DrawImage(mUncheckedImage, 0, 0, mCheckedRect);
		else
			g->DrawImage(mUncheckedImage, 0, 0, mUncheckedRect);
	}
	else if ((mUncheckedImage == NULL) && (mCheckedImage == NULL))
	{
		// No image, default draw method
		g->SetColor(mOutlineColor);
		g->FillRect(0, 0, mWidth, mHeight);
		g->SetColor(mBkgColor);
		g->FillRect(1, 1, mWidth - 2, mHeight - 2);

		if (mChecked)
		{
			g->SetColor(mCheckColor);
			g->DrawLine(1, 1, mWidth - 2, mHeight - 2);
			g->DrawLine(mWidth - 1, 1, 1, mHeight - 2);
		}
	}
}

// FUNCTION: GAME187 0x485f20
void Checkbox::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	Widget::MouseDown(x, y, theBtnNum, theClickCount);

	mChecked = !mChecked;
	if (mListener)
		mListener->CheckboxChecked(mId, mChecked);
	MarkDirty();
}
