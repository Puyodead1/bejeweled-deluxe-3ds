#include "HighScoreDialog.h"
#include <stdio.h>
#include "WinDMApp.h"
#include "HighScoreWidget.h"
#include "SexyAppFramework/Graphics.h"
#include "SexyAppFramework/Font.h"
#include "SexyAppFramework/Color.h"

using namespace Sexy;

// FUNCTION: GAME 0x405b8d
// FUNCTION: GAME187 0x4ce44f
HighScoreDialog::~HighScoreDialog()
{
	delete mHighScoreWidget;
}

// FUNCTION: GAME 0x405be6
// FUNCTION: GAME187 0x4ce495
int HighScoreDialog::GetPreferredHeight(int theWidth)
{
	return 326;
}

// FUNCTION: GAME 0x405c16
// FUNCTION: GAME187 0x4ce4bc
HighScoreDialog::HighScoreDialog(WinDMApp* theApp)
	: Dialog(theApp->mImages[10], theApp->mImages[11], 13, true, theApp->GetString("HSDLG_HIGH_SCORES"), "", theApp->GetString("HSDLG_DONE"), 3)
{
	mHighScoreWidget = new HighScoreWidget(theApp);
	mApp = theApp;

	SetButtonFont(theApp->m_unk0x690);
	SetHeaderFont(mApp->m_unk0x68c);
	SetLinesFont(mApp->m_unk0x690);
}

// FUNCTION: GAME 0x405d65
// FUNCTION: GAME187 0x4ce60b
void HighScoreDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	g->SetFont(mApp->m_unk0x690);

	char aBuffer[256];
	for (int i = 0; i < 2; i++)
	{
		int aCenterX = mWidth / 2 + ((i == 0) ? -1 : 1) * 90;

		g->SetFont(mApp->m_unk0x690);

		std::string aString;

		g->SetColor(Color(255, 255, 128));
		aString = (i == 0) ? mApp->GetString("HSDLG_NORMAL") : mApp->GetString("HSDLG_TIMED");
		int aWidth = g->GetFont()->StringWidth(aString);
		int aX = aCenterX - aWidth / 2;
		g->DrawString(aString, aX, 73);
		g->FillRect(aX, 76, aWidth, 1);

		aString = mApp->GetString("HSDLG_TODAYS_HIGH_SCORE");
		aWidth = g->GetFont()->StringWidth(aString);
		aX = aCenterX - aWidth / 2;
		g->DrawString(aString, aX, 99);
		g->FillRect(aX, 102, aWidth, 1);

		g->SetColor(Color(255, 255, 0));
		if (mHighScoreWidget->mTodayScores[i] > 0)
		{
			aString = mHighScoreWidget->mTodayNames[i].c_str();
			g->DrawString(aString, aCenterX - 74, 125);

			sprintf(aBuffer, "%d", mHighScoreWidget->mTodayScores[i]);
			aWidth = g->GetFont()->StringWidth(aBuffer);
			g->DrawString(aBuffer, aCenterX - aWidth + 74, 125);
		}
		else
		{
			aString = mApp->GetString("HSDLG_EMPTY");
			g->DrawString(aString, aCenterX - g->GetFont()->StringWidth(aString) / 2, 125);
		}

		g->SetColor(Color(255, 255, 128));
		aString = mApp->GetString("HSDLG_ALL_TIME_HIGH_SCORES");
		aWidth = g->GetFont()->StringWidth(aString);
		aX = aCenterX - aWidth / 2;
		g->DrawString(aString, aX, 155);
		g->FillRect(aX, 158, aWidth, 1);

		g->SetColor(Color(255, 255, 0));
		for (int j = 0; j < 5; j++)
		{
			int aY = 179 + j * 17;

			aString = mHighScoreWidget->mNames[i][j].c_str();
			g->DrawString(aString, aCenterX - 74, aY);

			sprintf(aBuffer, "%d", mHighScoreWidget->mScores[i][j]);
			aWidth = g->GetFont()->StringWidth(aBuffer);
			g->DrawString(aBuffer, aCenterX - aWidth + 74, aY);
		}
	}
}