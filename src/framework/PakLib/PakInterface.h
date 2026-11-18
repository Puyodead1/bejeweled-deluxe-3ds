#ifndef __PAKINTERFACE_H__
#define __PAKINTERFACE_H__

// Switch port: the Steam 1.87 build ships loose files (no main.pak), so the pak reader is gone and these are thin
// wrappers over stdio. Relative game paths ("images\\Gems.gif") are mapped into RomFS by Sexy::ResolveDataPath:
// backslashes become '/', the path is lowercased (tools/pack_data.ps1 stores RomFS in lowercase) and "romfs:/" is
// prepended. Absolute paths ("sdmc:/...", "romfs:/...") are used as they are.

#include <cstdio>
#include <cwchar>
#include <string>

namespace Sexy
{
std::string ResolveDataPath(const std::string& thePath);
}

struct PFILE
{
	FILE* mFP;
};

class PakInterface
{
public:
	bool AddPakFile(const std::string&) { return false; }
};

extern PakInterface* gPakInterface;

PFILE*		p_fopen(const char* theFileName, const char* theAccess);
PFILE*		p_fopen(const wchar_t* theFileName, const wchar_t* theAccess);
int			p_fclose(PFILE* theFile);
int			p_fseek(PFILE* theFile, long theOffset, int theOrigin);
int			p_ftell(PFILE* theFile);
size_t		p_fread(void* thePtr, int theSize, int theCount, PFILE* theFile);
size_t		p_fwrite(const void* thePtr, int theSize, int theCount, PFILE* theFile);
int			p_fgetc(PFILE* theFile);
int			p_ungetc(int theChar, PFILE* theFile);
char*		p_fgets(char* thePtr, int theSize, PFILE* theFile);
wchar_t*	p_fgets(wchar_t* thePtr, int theSize, PFILE* theFile);
int			p_feof(PFILE* theFile);

#endif //__PAKINTERFACE_H__
