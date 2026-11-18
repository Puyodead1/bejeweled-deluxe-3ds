#ifndef __MIGRATIONMANAGER_H__
#define __MIGRATIONMANAGER_H__

#include <string>
#include <list>

namespace Sexy
{

// 1.87: on Vista the game's writable data moves from the install directory to GetAppDataFolder().
// Files queued here are moved (or copied, for directories) from the old location on first run.
// The original has no symbol for this class; the name is ours.
class MigrationManager
{
public:
	typedef std::pair<std::string, std::string> MigratePair;
	typedef std::list<MigratePair> MigrateList;

	MigrateList				mMigrateList;

public:
	void					AddFile(const std::string& theFileName);
	void					DoMigration();
};

}

#endif //__MIGRATIONMANAGER_H__
