#ifndef __CtrRegistry_H__
#define __CtrRegistry_H__

// Stand-in for the Win32 registry: keys are backslash paths ("SOFTWARE\PopCap\Bejeweled\..."), compared without
// case, each holding typed values. The whole tree lives in memory and is rewritten to one file on every change.

#include <stdint.h>
#include <string>
#include <vector>

namespace CtrRegistry
{
	// Where the tree is saved; set before the first access (defaults to sdmc:/3ds/Bejeweled/registry.dat).
	void		SetFile(const std::string& thePath);

	bool		Read(const std::string& theKey, const std::string& theValueName, uint32_t* theType, uint8_t* theData, uint32_t* theLength);
	bool		Write(const std::string& theKey, const std::string& theValueName, uint32_t theType, const uint8_t* theData, uint32_t theLength);
	bool		DeleteKey(const std::string& theKey);
	bool		DeleteValue(const std::string& theKey, const std::string& theValueName);
	bool		GetSubKeys(const std::string& theKey, std::vector<std::string>* theSubKeys);

	// While one of these is alive, changes are kept in memory and the file is rewritten once at the end, instead of
	// once per value (WriteToRegistry sets a dozen values in a row).
	struct Batch
	{
		Batch();
		~Batch();
	};
}

#endif
