#ifndef _FDS_MIGRATOR_H
#define _FDS_MIGRATOR_H

namespace fds {

/* Global migration service singleton */
class FdsMigrationSvc;
extern FdsMigrationSvc *g_migrationSvc;

/**
 * Base migrator class
 */
class FdsMigrator {
public:
    FdsMigrator(const std::string &migration_id) {
        migration_id_ = migration_id;
    }

    virtual ~FdsMigrator() {
    }

    std::string get_migration_id() {
        return migration_id_;
    }
protected:
    /* Migration identifier */
    std::string migration_id_;
};

/**
 * Base class for sender side of migration
 */
class MigrationSender : public FdsMigrator {
public:
    MigrationSender(const std::string &migration_id)
    : FdsMigrator(migration_id)
    {
    }
};

/**
 * Base class for receiver side of migration
 */
class MigrationReceiver : public FdsMigrator {
public:
    MigrationReceiver(const std::string &migration_id)
    : FdsMigrator(migration_id)
    {
    }
};

}  // namespace fds

#endif
