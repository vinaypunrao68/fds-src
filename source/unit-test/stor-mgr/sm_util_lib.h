/**
* Copyright 2014 Formation Data Systems, Inc.
*/

#include "sm_dataset.h"
#include <unistd.h>

class FdsRootDir;
class DLT;

namespace fds {
    /** Class for managing external dependencies and structures for running stand-alone SM.
    * These operations may be destructive, and should only be used with care in a test environment.
    */
    class UnsafeSmMgmt {

    public:
        static fds_bool_t setupFreshDiskMap(const FdsRootDir *dir, fds_uint32_t simHddCount,
                fds_uint32_t simSsdCount);

        static void cleanFdsTestDev(FdsRootDir *dir);

    private:


    };

    /** Class for managing external dependencies and structures for running stand-alone SM.
    *
    */
    class SafeSmMgmt {

    public:
        static void getDiskMap(const FdsRootDir *dir,
                std::unordered_map <fds_uint16_t, std::string> *diskMap);
        static fds_bool_t diskMapPresent(const FdsRootDir *dir);
        static fds_bool_t setupDiskMap(const FdsRootDir *dir, fds_uint_32_t simHddCount,
                fds_uint32_t simSsdCount);
        static void populateDlt(DLT *dlt, fds_uint32_t sm_count);
        static DLT* newDLT(fds_uint32_t sm_count);
    };

    /** Class for bringing up a stand-alone SM for functional testing.
    *
    */
    class StandaloneSM : public FdsProcess {
    public:
        StandaloneSM(int argc, char * argv[], const std::string & config,
                const std::string & basePath,
                Module * vec[]) : FdsProcess(argc, argv, config, basePath, vec) {}
        virtual ~StandaloneSM() {}
        virtual int run() override;

        // Data
        static ObjectStore::unique_ptr objectStore;
        static fds::Module *smVec[] = {
                &diskio::gl_dataIOMod,
                objectStore.get(),
                nullptr
        };
        Error populate();
    private:
        static StorMgrVolumeTable* volTbl;
        static fds_volid_t singleVolId = 74;
        TestVolume::ptr volume_;
    };

}  // namespace fds