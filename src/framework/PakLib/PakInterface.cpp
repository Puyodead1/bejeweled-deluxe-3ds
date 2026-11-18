#include "PakInterface.h"

#include <cctype>
#include <cstring>
#include <cwchar>

PakInterface* gPakInterface = new PakInterface();

std::string Sexy::ResolveDataPath(const std::string& thePath)
{
	std::string aPath = thePath;
	for (char& c : aPath)
	{
		if (c == '\\')
			c = '/';
	}

	// Already a device path (sdmc:/, romfs:/): leave it alone, case included.
	size_t aColon = aPath.find(':');
	if (aColon != std::string::npos && aColon < aPath.find('/'))
		return aPath;

	while (aPath.compare(0, 2, "./") == 0)
		aPath.erase(0, 2);
	while (!aPath.empty() && aPath[0] == '/')
		aPath.erase(0, 1);
	for (char& c : aPath)
		c = (char)tolower((unsigned char)c);
	return "romfs:/" + aPath;
}

static PFILE* WrapFile(FILE* theFP)
{
	if (theFP == NULL)
		return NULL;
	PFILE* aPFile = new PFILE();
	aPFile->mFP = theFP;
	return aPFile;
}

PFILE* p_fopen(const char* theFileName, const char* theAccess)
{
	return WrapFile(fopen(Sexy::ResolveDataPath(theFileName).c_str(), theAccess));
}

PFILE* p_fopen(const wchar_t* theFileName, const wchar_t* theAccess)
{
	std::string aName, anAccess;
	for (const wchar_t* p = theFileName; *p; p++)
		aName += (char)*p;
	for (const wchar_t* p = theAccess; *p; p++)
		anAccess += (char)*p;
	return p_fopen(aName.c_str(), anAccess.c_str());
}

int p_fclose(PFILE* theFile)
{
	int aResult = fclose(theFile->mFP);
	delete theFile;
	return aResult;
}

int p_fseek(PFILE* theFile, long theOffset, int theOrigin)
{
	return fseek(theFile->mFP, theOffset, theOrigin);
}

int p_ftell(PFILE* theFile)
{
	return (int)ftell(theFile->mFP);
}

size_t p_fread(void* thePtr, int theSize, int theCount, PFILE* theFile)
{
	return fread(thePtr, theSize, theCount, theFile->mFP);
}

size_t p_fwrite(const void* thePtr, int theSize, int theCount, PFILE* theFile)
{
	return fwrite(thePtr, theSize, theCount, theFile->mFP);
}

int p_fgetc(PFILE* theFile)
{
	return fgetc(theFile->mFP);
}

int p_ungetc(int theChar, PFILE* theFile)
{
	return ungetc(theChar, theFile->mFP);
}

char* p_fgets(char* thePtr, int theSize, PFILE* theFile)
{
	return fgets(thePtr, theSize, theFile->mFP);
}

wchar_t* p_fgets(wchar_t* thePtr, int theSize, PFILE* theFile)
{
	return fgetws(thePtr, theSize, theFile->mFP);
}

int p_feof(PFILE* theFile)
{
	return feof(theFile->mFP);
}
