#ifndef _FDS_MIGRATOR_H
#define _FDS_MIGRATOR_H

namespace fds {

/**
 * Base migrator class
 */
class FdsMigrator {
public:
    FdsMigrator(const std::string &mig_id) {
        mig_id_ = mig_id;
    }

    virtual ~FdsMigrator() {
    }

    std::string get_mig_id() {
        return mig_id_;
    }
protected:
    /* Migration identifier */
    std::string mig_id_;
};

/**
 * Base class for sender side of migration
 */
class MigrationSender : public FdsMigrator {
public:
    MigrationSender(const std::string &mig_id)
    : FdsMigrator(mig_id)
    {
    }
};

/**
 * Base class for receiver side of migration
 */
class MigrationReceiver : public FdsMigrator {
public:
    MigrationReceiver(const std::string &mig_id)
    : FdsMigrator(mig_id)
    {
    }
};

}  // namespace fds

#endif
