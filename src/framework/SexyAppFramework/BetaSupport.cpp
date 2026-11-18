#include "BetaSupport.h"
#include "SexyApp.h"
#include "Common.h"
#include <fstream>
#include <shellapi.h>

using namespace Sexy;

// FUNCTION: GAME187 0x46cca0
BetaSupport::BetaSupport(SexyApp* theApp)
{
	mApp = theApp;
	mTickCount = 0;
	mStatusHWnd = NULL;
	mCancelled = false;
	mDone = true;
	mHWnd = NULL;

	HWND aDesktopWnd = GetDesktopWindow();
	HDC aDC = GetDC(aDesktopWnd);
	int aHeight = MulDiv(8, 96, 72);
	mNormalFont = CreateFontA(-aHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
	aHeight = MulDiv(8, 96, 72);
	mSmallFont = CreateFontA(-aHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	aHeight = MulDiv(10, 96, 72);
	mBoldFont = CreateFontA(-aHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
	ReleaseDC(aDesktopWnd, aDC);

	mValidated = false;
}

// FUNCTION: GAME187 0x46ce60
BetaSupport::~BetaSupport()
{
	if (mHWnd != NULL)
	{
		MessageLoop();
		DestroyWindow(mHWnd);
		mHWnd = NULL;
	}

	DeleteObject(mNormalFont);
	DeleteObject(mSmallFont);
	DeleteObject(mBoldFont);

	MessageLoop();
}

// FUNCTION: GAME187 0x46cfb0
void BetaSupport::CenterWindow(HWND theHWnd)
{
	RECT aWorkArea;
	SystemParametersInfoA(SPI_GETWORKAREA, 0, &aWorkArea, 0);

	RECT aRect;
	GetWindowRect(theHWnd, &aRect);

	int aWidth = aRect.right - aRect.left;
	int aHeight = aRect.bottom - aRect.top;
	int aY = aWorkArea.bottom - aWorkArea.top - aHeight;

	MoveWindow(theHWnd,
		aWorkArea.left + (aWorkArea.right - aWorkArea.left - aWidth) / 2,
		aWorkArea.top - (int) (aY * -0.382),
		aWidth, aHeight, FALSE);
}

// FUNCTION: GAME187 0x46d030
void BetaSupport::MessageLoop()
{
	while (!mDone)
	{
		MSG aMsg;
		if (GetMessageA(&aMsg, NULL, 0, 0) <= 0)
			break;

		if (!IsDialogMessageA(mHWnd, &aMsg))
		{
			TranslateMessage(&aMsg);
			DispatchMessageA(&aMsg);
		}
	}
}

// FUNCTION: GAME187 0x46d220
static LRESULT CALLBACK LoginWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BetaSupport* aBetaSupport = (BetaSupport*) GetWindowLongA(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
	case WM_PAINT:
		{
			RECT aClientRect;
			GetClientRect(hWnd, &aClientRect);

			PAINTSTRUCT aPaintStruct;
			BeginPaint(hWnd, &aPaintStruct);

			HGDIOBJ anOldFont = SelectObject(aPaintStruct.hdc, aBetaSupport->mNormalFont);
			SetBkMode(aPaintStruct.hdc, TRANSPARENT);
			SetTextColor(aPaintStruct.hdc, RGB(0, 0, 0));

			char* aString = "Email Address";
			TextOutA(aPaintStruct.hdc, 6, 8, aString, strlen(aString));
			aString = "Password";
			TextOutA(aPaintStruct.hdc, 6, 53, aString, strlen(aString));

			SelectObject(aPaintStruct.hdc, aBetaSupport->mSmallFont);

			HBRUSH aBrush = CreateSolidBrush(RGB(0x71, 0x73, 0x97));
			RECT aLineRect = {4, 98, aClientRect.right - 4, 99};
			FillRect(aPaintStruct.hdc, &aLineRect, aBrush);
			DeleteObject(aBrush);

			SetTextColor(aPaintStruct.hdc, RGB(0x68, 0x1E, 0x1E));
			RECT aTextRect = {8, 108, 294, 300};
			DrawTextA(aPaintStruct.hdc,
				"This product is intended only for authorized members of\n"
				"the PopCap Beta program.  Any unauthorized distribution\n"
				"should be reported to beta@popcap.com.",
				-1, &aTextRect, 0);

			SelectObject(aPaintStruct.hdc, anOldFont);
			EndPaint(hWnd, &aPaintStruct);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1)
		{
			if (GetFocus() == aBetaSupport->mUserNameHWnd)
				SetFocus(aBetaSupport->mPasswordHWnd);
			else
				aBetaSupport->mDone = true;
		}
		else if (LOWORD(wParam) == 2)
		{
			aBetaSupport->mCancelled = true;
			aBetaSupport->mDone = true;
		}
		break;
	case WM_CLOSE:
		aBetaSupport->mCancelled = true;
		aBetaSupport->mDone = true;
		break;
	}

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

// FUNCTION: GAME187 0x46d530
void BetaSupport::SaveToRegistry()
{
	if (mApp->mPlayingDemoBuffer)
		return;

	HKEY aKey;
	DWORD aDisposition;
	if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\PopCap\\Beta", 0, "Key", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &aKey, &aDisposition) != ERROR_SUCCESS)
		return;

	RegSetValueExA(aKey, "Name", 0, REG_SZ, (const BYTE*) mUserName.c_str(), mUserName.length());
	RegSetValueExA(aKey, "Password", 0, REG_SZ, (const BYTE*) mPassword.c_str(), mPassword.length());
	RegCloseKey(aKey);
}

// FUNCTION: GAME187 0x46d0a0
static LRESULT CALLBACK ValidateWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BetaSupport* aBetaSupport = (BetaSupport*) GetWindowLongA(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
	case WM_CLOSE:
		aBetaSupport->mTransfer.Abort();
		return 0;
	case WM_COMMAND:
		if (LOWORD(wParam) == 2)
		{
			aBetaSupport->mTransfer.Abort();
			aBetaSupport->mCancelled = true;
		}
		break;
	case WM_TIMER:
		if (aBetaSupport != NULL)
		{
			if (wParam == 0)
			{
				if ((aBetaSupport->mTransfer.GetResultCode() != HTTPTransfer::RESULT_NOT_COMPLETED) &&
					(aBetaSupport->mTransfer.GetResultCode() != HTTPTransfer::RESULT_NOT_STARTED))
					aBetaSupport->mDone = true;
			}
			else if (wParam == 1)
			{
				aBetaSupport->mTickCount++;

				std::string aString = "Please Wait";
				for (int i = 0; i < aBetaSupport->mTickCount % 10; i++)
					aString += " .";

				SetWindowTextA(aBetaSupport->mStatusHWnd, aString.c_str());
			}
		}
		break;
	}

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

// FUNCTION: GAME187 0x46d420
void BetaSupport::LoadFromRegistry()
{
	if (mApp->mPlayingDemoBuffer)
		return;

	HKEY aKey;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\PopCap\\Beta", 0, KEY_READ, &aKey) != ERROR_SUCCESS)
		return;

	char aBuffer[256];
	DWORD aType;
	DWORD aLength;

	aLength = 256;
	if (RegQueryValueExA(aKey, "Name", NULL, &aType, (BYTE*) aBuffer, &aLength) == ERROR_SUCCESS)
		mUserName = aBuffer;

	aLength = 256;
	if (RegQueryValueExA(aKey, "Password", NULL, &aType, (BYTE*) aBuffer, &aLength) == ERROR_SUCCESS)
		mPassword = aBuffer;

	RegCloseKey(aKey);
}

#define BETA_WINDOW_STYLE (WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

// FUNCTION: GAME187 0x46d5f0
bool BetaSupport::Login()
{
	mDone = false;
	mCancelled = false;

	if (mApp->mPlayingDemoBuffer)
	{
		bool aCancelled = false;
		mApp->DemoSyncBool(&aCancelled);
		return !aCancelled;
	}

	LoadFromRegistry();

	WNDCLASSA aWndClass;
	aWndClass.style = 0;
	aWndClass.cbClsExtra = 0;
	aWndClass.cbWndExtra = 0;
	aWndClass.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	aWndClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
	aWndClass.hIcon = LoadIconA(NULL, IDI_QUESTION);
	aWndClass.hInstance = gHInstance;
	aWndClass.lpfnWndProc = LoginWindowProc;
	aWndClass.lpszClassName = "LoginWindow";
	aWndClass.lpszMenuName = NULL;
	RegisterClassA(&aWndClass);

	RECT aRect;
	aRect.left = 0;
	aRect.top = 0;
	aRect.right = 306;
	aRect.bottom = 162;
	AdjustWindowRect(&aRect, BETA_WINDOW_STYLE, FALSE);

	mHWnd = CreateWindowExA(0, "LoginWindow", "PopCap Beta Login...", BETA_WINDOW_STYLE,
		64, 64, aRect.right - aRect.left, aRect.bottom - aRect.top, NULL, NULL, gHInstance, NULL);
	SetWindowLongA(mHWnd, GWL_USERDATA, (LONG) this);
	SetTimer(mHWnd, 0, 20, NULL);
	SetTimer(mHWnd, 1, 1000, NULL);

	mUserNameHWnd = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", mUserName.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
		6, 20, 204, 22, mHWnd, NULL, gHInstance, NULL);
	SendMessageA(mUserNameHWnd, WM_SETFONT, (WPARAM) mNormalFont, 0);

	mPasswordHWnd = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", mPassword.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_PASSWORD,
		6, 65, 204, 22, mHWnd, NULL, gHInstance, NULL);
	SendMessageA(mPasswordHWnd, WM_SETFONT, (WPARAM) mNormalFont, 0);

	HWND aLoginButton = CreateWindowExA(0, "BUTTON", "Login", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
		222, 6, 72, 20, mHWnd, (HMENU) 1, gHInstance, NULL);
	SendMessageA(aLoginButton, WM_SETFONT, (WPARAM) mNormalFont, 0);

	HWND aCancelButton = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		222, 32, 72, 20, mHWnd, (HMENU) 2, gHInstance, NULL);
	SendMessageA(aCancelButton, WM_SETFONT, (WPARAM) mNormalFont, 0);

	CenterWindow(mHWnd);

	if (mPassword.length() > 0)
		SetFocus(aLoginButton);
	else
		SetFocus(mUserNameHWnd);

	ShowWindow(mHWnd, SW_SHOWNORMAL);

	MessageLoop();

	if (!mCancelled)
	{
		char aString[256];

		GetWindowTextA(mUserNameHWnd, aString, 256);
		mUserName = aString;

		GetWindowTextA(mPasswordHWnd, aString, 256);
		mPassword = aString;
	}

	DestroyWindow(mHWnd);
	mHWnd = NULL;

	mApp->DemoSyncBool(&mCancelled);
	return !mCancelled;
}

// FUNCTION: GAME187 0x46d930
bool BetaSupport::CheckValidation()
{
	mDone = false;
	mCancelled = false;

	std::string aURL = "http://www.popcap.com/beta_validate.php?prod=" + mApp->mProdName +
		"&version=" + mApp->mProductVersion +
		"&username=" + URLEncode(mUserName) +
		"&pass=" + URLEncode(mPassword);
	mTransfer.Get(aURL);

	WNDCLASSA aWndClass;
	aWndClass.style = 0;
	aWndClass.cbClsExtra = 0;
	aWndClass.cbWndExtra = 0;
	aWndClass.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	aWndClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
	aWndClass.hIcon = LoadIconA(NULL, IDI_INFORMATION);
	aWndClass.hInstance = gHInstance;
	aWndClass.lpfnWndProc = ValidateWindowProc;
	aWndClass.lpszClassName = "ValidateWindow";
	aWndClass.lpszMenuName = NULL;
	RegisterClassA(&aWndClass);

	RECT aRect;
	aRect.left = 0;
	aRect.top = 0;
	aRect.right = 240;
	aRect.bottom = 64;
	AdjustWindowRect(&aRect, BETA_WINDOW_STYLE, FALSE);

	mHWnd = CreateWindowExA(0, "ValidateWindow", "Validating Beta", BETA_WINDOW_STYLE,
		64, 64, aRect.right - aRect.left, aRect.bottom - aRect.top, NULL, NULL, gHInstance, NULL);
	SetWindowLongA(mHWnd, GWL_USERDATA, (LONG) this);
	SetTimer(mHWnd, 0, 20, NULL);
	SetTimer(mHWnd, 1, 1000, NULL);

	mStatusHWnd = CreateWindowExA(0, "EDIT", "Please Wait", WS_CHILD | WS_VISIBLE | ES_READONLY,
		20, 10, 224, 24, mHWnd, NULL, gHInstance, NULL);
	SendMessageA(mStatusHWnd, WM_SETFONT, (WPARAM) mBoldFont, 0);

	HWND anAbortButton = CreateWindowExA(0, "BUTTON", "Abort", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
		72, 36, 96, 22, mHWnd, (HMENU) 2, gHInstance, NULL);
	SendMessageA(anAbortButton, WM_SETFONT, (WPARAM) mNormalFont, 0);

	CenterWindow(mHWnd);
	ShowWindow(mHWnd, SW_SHOWNORMAL);

	MessageLoop();

	DestroyWindow(mHWnd);
	mHWnd = NULL;

	if (mTransfer.GetResultCode() == HTTPTransfer::RESULT_DONE)
	{
		std::string aContent = mTransfer.GetContent();

		Buffer aBuffer;
		aBuffer.SetData((uchar*) aContent.c_str(), aContent.length());

		std::string aResult = aBuffer.ReadLine();

		bool hadError = false;
		std::string anError;
		std::string anUpdateURL;

		while (!aBuffer.AtEnd())
		{
			std::string aLine = aBuffer.ReadLine();
			std::string aValue;

			int aSpacePos = aLine.find(' ');
			if (aSpacePos != -1)
			{
				aValue = aLine.substr(aSpacePos + 1);
				aLine = aLine.substr(0, aSpacePos);
			}

			if (aLine == ":ERROR")
				anError = aValue;
			else if (aLine == ":UPDATEURL")
				anUpdateURL = aValue;
			else if (aLine == ":COMMENTURL")
				mCommentURL = aValue;
			else if (aLine.length() > 0)
				hadError = true;
		}

		bool aSucceeded = aResult == "SUCCEEDED";
		if ((!aSucceeded) && (aResult != "FAILED"))
			hadError = true;

		if (hadError)
		{
			MessageBoxA(NULL, "Unable to parse Beta Validation results! Check your internet connection and try again", "Validation Failed!", MB_ICONERROR);
			return false;
		}

		if (aResult == "SUCCEEDED")
			return true;

		{
			if (aResult == "FAILED")
			{
				if ((anError == "Update Needed") && (anUpdateURL.substr(0, 7) == "http://"))
				{
					if (MessageBoxA(NULL, "A new version of this product is now available. Click OK to open a page to download it from.", "Update Required!", MB_ICONWARNING) == IDOK)
					{
						ShellExecuteA(NULL, "open", anUpdateURL.c_str(), NULL, NULL, SW_SHOWNORMAL);
						return false;
					}
				}
				else
				{
					if (anError == "Server Error")
						MessageBoxA(NULL, "Internal Server Error! Try again later.", "Server Error", MB_ICONINFORMATION);
					else if (anError == "Beta Discontinued")
						MessageBoxA(NULL, "This product has been removed from the PopCap Beta program.\nThanks for helping us test during the beta period!", "Beta Discontinued", MB_ICONINFORMATION);
					else if (anError == "Invalid Username")
						MessageBoxA(NULL, "Unable to locate a PopCap Beta account for the supplied user name.", "Invalid User Name!", MB_ICONERROR);
					else if (anError == "Account Disabled")
						MessageBoxA(NULL, "This PopCap Beta account has been disabled!", "Account Disabled!", MB_ICONERROR);
					else if (anError == "Invalid Password")
						MessageBoxA(NULL, "The password you supplied did not match this account.", "Invalid Password!", MB_ICONERROR);
					else
						MessageBoxA(NULL, anError.c_str(), "Error!", MB_ICONERROR);
					return false;
				}
			}
		}

		return true;
	}
	else if (mTransfer.GetResultCode() != HTTPTransfer::RESULT_ABORTED)
	{
		MessageBoxA(NULL, "Beta Validation failed! Check your internet connection and try again.", "Validation Failed!", MB_ICONERROR);
	}

	return false;
}

// FUNCTION: GAME187 0x46e5d0
bool BetaSupport::Validate()
{
	if (!Login())
		return false;

	mApp->DemoSyncString(&mUserName);
	mApp->DemoSyncString(&mPassword);
	mApp->mUserName = mUserName;

	if (!CheckValidation())
		return false;

	SaveToRegistry();
	mValidated = true;
	return true;
}
