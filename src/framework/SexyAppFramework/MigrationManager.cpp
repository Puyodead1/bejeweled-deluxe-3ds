#include "MigrationManager.h"
#include "Common.h"
#include <windows.h>

using namespace Sexy;

// Queue theFileName (relative to the install directory) for moving to the per-user data folder,
// if it exists at the old location and not yet at the new one.
// FUNCTION: GAME187 0x4744c0
void MigrationManager::AddFile(const std::string& theFileName)
{
	if (!CheckForVista())
		return;

	std::string anOldPath = RemoveTrailingSlash(GetCurDir()) + "\\" + theFileName;
	std::string aNewPath = GetAppDataFolder() + theFileName;

	if (FileExists(anOldPath) && !FileExists(aNewPath))
	{
		MigratePair aPair;
		aPair.first = anOldPath;
		aPair.second = aNewPath;
		mMigrateList.push_back(aPair);
	}
}

// FUNCTION: GAME187 0x474760
void MigrationManager::DoMigration()
{
	for (MigrateList::iterator anItr = mMigrateList.begin(); anItr != mMigrateList.end(); ++anItr)
	{
		MigratePair& aPair = *anItr;
		if (!MoveFileExA(aPair.first.c_str(), aPair.second.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
		{
			WIN32_FIND_DATAA aFindData;
			HANDLE aFindHandle = FindFirstFileA(aPair.first.c_str(), &aFindData);
			if (aFindHandle != INVALID_HANDLE_VALUE)
			{
				if (aFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					FindClose(aFindHandle);

					// Copy the directory tree breadth-first; the old copy is left in place.
					std::list<std::string> aDirList;
					aDirList.push_back("");
					while (aDirList.size() != 0)
					{
						std::string aDir = aDirList.front();
						aDirList.pop_front();

						HANDLE aDirHandle = FindFirstFileA((aPair.first + "\\" + aDir + "\\*.*").c_str(), &aFindData);
						if (aDirHandle != INVALID_HANDLE_VALUE)
						{
							MkDir(aPair.second + "\\" + aDir);
							do
							{
								std::string aName = aFindData.cFileName;
								if ((aName != "..") && (aName != "."))
								{
									if (aFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
									{
										if (aDir.length() != 0)
											aDirList.push_back(aDir + "\\" + aName);
										else
											aDirList.push_back(aName);
									}
									else
									{
										CopyFileA((aPair.first + "\\" + aDir + "\\" + aName).c_str(), (aPair.second + "\\" + aDir + "\\" + aName).c_str(), TRUE);
									}
								}
							}
							while (FindNextFileA(aDirHandle, &aFindData));
							FindClose(aDirHandle);
						}
					}
				}
				else
				{
					FindClose(aFindHandle);
					CopyFileA(aPair.first.c_str(), aPair.second.c_str(), TRUE);
				}
			}
		}
	}
}
