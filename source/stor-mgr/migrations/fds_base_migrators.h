#ifndef _FDS_MIGRATOR_H
#define _FDS_MIGRATOR_H

namespace fds {

class FdsMigrator {
public:
    FdsMigrator() {
    }
    virtual ~FdsMigrator() {
    }
};

class MigrationSender : public FdsMigrator {
public:
    MigrationSender()
    {
    }
};

class MigrationReceiver : public FdsMigrator {
public:
    MigrationReceiver()
    {
    }
};

}  // namespace fds

#endif
