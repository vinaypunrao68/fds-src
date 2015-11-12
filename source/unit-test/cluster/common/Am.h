#ifndef AM_H_
#define AM_H_

/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <net/SvcProcess.h>
#include <VolumeGroupHandle.h>

namespace fds {
struct VolumeGroupHandle;

struct AmProcess : SvcProcess {
    AmProcess(int argc, char *argv[], bool initAsModule);
    void attachVolume(const fpi::VolumeGroupInfo &groupInfo);
    void putBlob(const fds_volid_t &volId);
    void putBlob(const fds_volid_t &volId, const VolumeResponseCb &cb);
    int run() override;

 protected:
    SHPTR<VolumeGroupHandle>    volHandle_;
    int64_t                     txId_;
};

}  // namespace fds
#endif
