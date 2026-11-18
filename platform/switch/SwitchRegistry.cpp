#include "SwitchRegistry.h"

#include <map>
#include <set>
#include <mutex>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

// File format (text, one entry per line):
//   <key>\t<value name>\t<type>\t<hex bytes>
// Keys and value names never contain tabs or newlines in this game.

namespace
{
	struct Value
	{
		uint32_t				mType;
		std::vector<uint8_t>	mData;
	};

	struct NoCaseLess
	{
		bool operator()(const std::string& a, const std::string& b) const { return strcasecmp(a.c_str(), b.c_str()) < 0; }
	};

	typedef std::map<std::string, Value, NoCaseLess> ValueMap;
	typedef std::map<std::string, ValueMap, NoCaseLess> KeyMap;

	std::recursive_mutex	gMutex;
	KeyMap					gKeys;
	std::string				gFile = "sdmc:/switch/Bejeweled/registry.dat";
	bool					gLoaded = false;
	int						gBatchDepth = 0;
	bool					gDirty = false;

	std::string Normalize(const std::string& theKey)
	{
		std::string aKey = theKey;
		for (size_t i = 0; i < aKey.size(); i++)
			if (aKey[i] == '/')
				aKey[i] = '\\';
		while (!aKey.empty() && aKey[aKey.size() - 1] == '\\')
			aKey.erase(aKey.size() - 1);
		return aKey;
	}

	void MakeDirs(const std::string& thePath)
	{
		// Create every directory above thePath, skipping a device root such as "sdmc:".
		for (size_t i = 0; i < thePath.size(); i++)
		{
			if (thePath[i] != '/' || i == 0 || thePath[i - 1] == ':')
				continue;
			mkdir(thePath.substr(0, i).c_str(), 0777);
		}
	}

	void Load()
	{
		if (gLoaded)
			return;
		gLoaded = true;

		FILE* aFP = fopen(gFile.c_str(), "rb");
		if (aFP == NULL)
			return;

		std::string aLine;
		int aChar;
		for (;;)
		{
			aChar = fgetc(aFP);
			if (aChar != '\n' && aChar != EOF)
			{
				aLine += (char)aChar;
				continue;
			}

			size_t aTab1 = aLine.find('\t');
			size_t aTab2 = aTab1 == std::string::npos ? aTab1 : aLine.find('\t', aTab1 + 1);
			size_t aTab3 = aTab2 == std::string::npos ? aTab2 : aLine.find('\t', aTab2 + 1);
			if (aTab3 != std::string::npos)
			{
				Value aValue;
				aValue.mType = (uint32_t)strtoul(aLine.substr(aTab2 + 1, aTab3 - aTab2 - 1).c_str(), NULL, 10);
				const std::string aHex = aLine.substr(aTab3 + 1);
				for (size_t i = 0; i + 1 < aHex.size(); i += 2)
					aValue.mData.push_back((uint8_t)strtoul(aHex.substr(i, 2).c_str(), NULL, 16));
				gKeys[aLine.substr(0, aTab1)][aLine.substr(aTab1 + 1, aTab2 - aTab1 - 1)] = aValue;
			}

			aLine.clear();
			if (aChar == EOF)
				break;
		}
		fclose(aFP);
	}

	void Save()
	{
		if (gBatchDepth > 0)
		{
			gDirty = true;
			return;
		}
		gDirty = false;

		MakeDirs(gFile);

		std::string aTemp = gFile + ".tmp";
		FILE* aFP = fopen(aTemp.c_str(), "wb");
		if (aFP == NULL)
			return;

		for (KeyMap::const_iterator aKey = gKeys.begin(); aKey != gKeys.end(); ++aKey)
		{
			for (ValueMap::const_iterator aVal = aKey->second.begin(); aVal != aKey->second.end(); ++aVal)
			{
				fprintf(aFP, "%s\t%s\t%u\t", aKey->first.c_str(), aVal->first.c_str(), (unsigned)aVal->second.mType);
				for (size_t i = 0; i < aVal->second.mData.size(); i++)
					fprintf(aFP, "%02x", aVal->second.mData[i]);
				fputc('\n', aFP);
			}
		}
		bool ok = !ferror(aFP);
		if (fclose(aFP) != 0 || !ok)
		{
			remove(aTemp.c_str());
			return;
		}

		// FAT can't rename over a file; SwitchPlatform::RecoverFiles finishes this if it's cut short.
		remove(gFile.c_str());
		rename(aTemp.c_str(), gFile.c_str());
	}
}

void SwitchRegistry::SetFile(const std::string& thePath)
{
	std::lock_guard<std::recursive_mutex> aLock(gMutex);
	gFile = thePath;
	gLoaded = false;
	gKeys.clear();
}

bool SwitchRegistry::Read(const std::string& theKey, const std::string& theValueName, uint32_t* theType, uint8_t* theData, uint32_t* theLength)
{
	std::lock_guard<std::recursive_mutex> aLock(gMutex);
	Load();

	KeyMap::const_iterator aKey = gKeys.find(Normalize(theKey));
	if (aKey == gKeys.end())
		return false;
	ValueMap::const_iterator aVal = aKey->second.find(theValueName);
	if (aVal == aKey->second.end())
		return false;

	// Like RegQueryValueEx: fail with the needed size when the buffer is too small.
	uint32_t aSize = (uint32_t)aVal->second.mData.size();
	if (theType != NULL)
		*theType = aVal->second.mType;
	if (aSize > *theLength)
	{
		*theLength = aSize;
		return false;
	}
	if (aSize > 0)
		memcpy(theData, &aVal->second.mData[0], aSize);
	*theLength = aSize;
	return true;
}

bool SwitchRegistry::Write(const std::string& theKey, const std::string& theValueName, uint32_t theType, const uint8_t* theData, uint32_t theLength)
{
	std::lock_guard<std::recursive_mutex> aLock(gMutex);
	Load();

	Value& aValue = gKeys[Normalize(theKey)][theValueName];
	aValue.mType = theType;
	aValue.mData.assign(theData, theData + theLength);
	Save();
	return true;
}

bool SwitchRegistry::DeleteKey(const std::string& theKey)
{
	std::lock_guard<std::recursive_mutex> aLock(gMutex);
	Load();

	// RegDeleteKey only removes a key without subkeys; the game only erases leaf keys, so remove the subtree.
	std::string aKey = Normalize(theKey);
	std::string aPrefix = aKey + "\\";
	bool found = false;
	for (KeyMap::iterator anItr = gKeys.begin(); anItr != gKeys.end(); )
	{
		if (strcasecmp(anItr->first.c_str(), aKey.c_str()) == 0 ||
			strncasecmp(anItr->first.c_str(), aPrefix.c_str(), aPrefix.size()) == 0)
		{
			gKeys.erase(anItr++);
			found = true;
		}
		else
			++anItr;
	}
	if (found)
		Save();
	return found;
}

bool SwitchRegistry::DeleteValue(const std::string& theKey, const std::string& theValueName)
{
	std::lock_guard<std::recursive_mutex> aLock(gMutex);
	Load();

	KeyMap::iterator aKey = gKeys.find(Normalize(theKey));
	if (aKey == gKeys.end() || aKey->second.erase(theValueName) == 0)
		return false;
	if (aKey->second.empty())
		gKeys.erase(aKey);
	Save();
	return true;
}

bool SwitchRegistry::GetSubKeys(const std::string& theKey, std::vector<std::string>* theSubKeys)
{
	std::lock_guard<std::recursive_mutex> aLock(gMutex);
	Load();

	theSubKeys->clear();
	std::string aPrefix = Normalize(theKey) + "\\";
	std::set<std::string, NoCaseLess> aSeen;
	bool exists = false;
	for (KeyMap::const_iterator anItr = gKeys.begin(); anItr != gKeys.end(); ++anItr)
	{
		if (strcasecmp(anItr->first.c_str(), Normalize(theKey).c_str()) == 0)
			exists = true;
		if (strncasecmp(anItr->first.c_str(), aPrefix.c_str(), aPrefix.size()) != 0)
			continue;
		exists = true;
		std::string aRest = anItr->first.substr(aPrefix.size());
		size_t aSlash = aRest.find('\\');
		if (aSlash != std::string::npos)
			aRest = aRest.substr(0, aSlash);
		if (aSeen.insert(aRest).second)
			theSubKeys->push_back(aRest);
	}
	return exists;
}

SwitchRegistry::Batch::Batch()
{
	gMutex.lock();
	gBatchDepth++;
}

SwitchRegistry::Batch::~Batch()
{
	if (--gBatchDepth == 0 && gDirty)
		Save();
	gMutex.unlock();
}
