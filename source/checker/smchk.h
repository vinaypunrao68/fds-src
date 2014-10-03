/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_CHECKER_SMCHK_H_
#define SOURCE_CHECKER_SMCHK_H_

class ObjectStore;
class SmDiskMap;
class ObjectMetadataDb;

namespace fds {

    class SMChk : public FdsProcess {
    friend class ObjMeta;
    public:
        SMChk(int argc, char * argv[], const std::string &config,
                const std::string &basePath, Module * vec[]);
        ~SMChk() {
        }

        int run() override;
        void list_path_by_token();
        void list_token_by_path();
        bool full_consistency_check();
        // bool consistency_check(ObjectId obj_id);  // test a single object

    private:
        // Data
        int sm_count;
        int error_count;
        // fds::SmDiskMap::ptr smDiskMap;
        // fds::ObjectDataStore::unique_ptr smObjStore;
        fds::ObjectMetadataDb::unique_ptr smMdDb;

        // Methods
        SmTokenSet getSmTokens();
    };

}  // namespace fds
#endif  // SOURCE_PLATFORM_INCLUDE_DISK_H_
