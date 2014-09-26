/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdio>
#include <string>
#include <vector>
#include <bitset>

#include <util/Log.h>
#include <fds_types.h>
#include <ObjectId.h>
#include <fds_module.h>
#include <fds_process.h>
#include <object-store/ObjectStore.h>
#include <concurrency/ThreadPool.h>
#include <sm_ut_utils.h>

namespace fds {

static const fds_uint32_t MAX_TEST_OBJ = 1000;
static const fds_uint32_t MAX_VOLUMES = 50;

static fds_uint32_t keyCounter = 0;

static fds_threadpool pool;

static fds_volid_t singleVolId = 555;

struct TestObject {
    fds_volid_t  volId;
    ObjectID     objId;
    boost::shared_ptr<std::string> value;

    TestObject() :
            // volId(keyCounter % MAX_VOLUMES),
            volId(singleVolId),
            value(new std::string(std::to_string(keyCounter++))) {
                objId = ObjIdGen::genObjectId(value->c_str(),
                                              value->size());
            }
    ~TestObject() {
    }
};

static StorMgrVolumeTable* volTbl;
static ObjectStore::unique_ptr objectStore;

static void getObj(TestObject & obj) {
    Error err(ERR_OK);
    boost::shared_ptr<const std::string> ptr
            = objectStore->getObject(obj.volId, obj.objId, err);
    GLOGTRACE << "compare " << *obj.value << " = " << *ptr;
    fds_assert(*ptr == *obj.value);
}

static void addObj(TestObject & obj) {
    GLOGTRACE << "Adding value " << *obj.value << " with id "
              << obj.objId;
    objectStore->putObject(obj.volId, obj.objId, obj.value);
    pool.schedule(getObj, obj, 0);
}

static void delObj(TestObject & obj) {
    Error err(ERR_OK);
    GLOGTRACE << "Deleting object with id " << obj.objId;
    err = objectStore->deleteObject(obj.volId, obj.objId);
    fds_assert(err.ok());
}
static std::vector<TestObject> testObjs(MAX_TEST_OBJ);

class ObjectStoreTest : public FdsProcess {
  public:
    ObjectStoreTest(int argc, char *argv[],
                    Module **mod_vec) :
            FdsProcess(argc,
                       argv,
                       "platform.conf",
                       "fds.sm.",
                       "objstore-test.log",
                       mod_vec) {
    }
    ~ObjectStoreTest() {
    }
    int run() override {
        fds_uint32_t sm_count =1;
        fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
        DLT* dlt = new DLT(16, cols, 1, true);
        SmUtUtils::populateDlt(dlt, sm_count);
        objectStore->handleNewDlt(dlt);
        LOGTRACE << "Starting...";

        VolumeDesc vdesc("objectstore_ut_volume", singleVolId);
        volTbl->registerVolume(vdesc);

        for (std::vector<TestObject>::iterator i = testObjs.begin();
             testObjs.end() != i;
             ++i) {
            LOGTRACE << "Writing: Details volume=" << (*i).volId << " key="
                     << (*i).objId << " value=" << *(*i).value;
            addObj(*i);
        }

        // read objects we just wrote
        for (std::vector<TestObject>::iterator i = testObjs.begin();
             testObjs.end() != i;
             ++i) {
            LOGTRACE << "Reading: Details volume=" << (*i).volId << " key="
                     << (*i).objId << " value=" << *(*i).value;
            getObj(*i);
        }

        // delete all objects
        for (std::vector<TestObject>::iterator i = testObjs.begin();
             testObjs.end() != i;
             ++i) {
            LOGTRACE << "Deleting: Details volume=" << (*i).volId << " key="
                     << (*i).objId;
            delObj(*i);
        }

        // delete object that is already deleted
        delObj(testObjs[0]);

        LOGTRACE << "Ending...";
        return 0;
    }
};

static void getObject() {
}

}  // namespace fds

int
main(int argc, char** argv) {
    volTbl = new StorMgrVolumeTable();
    objectStore = ObjectStore::unique_ptr(
        new ObjectStore("Unit Test Object Store", volTbl));

    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        objectStore.get(),
        nullptr
    };
    fds::ObjectStoreTest objectStoreTest(argc, argv, smVec);
    return objectStoreTest.main();

    return 0;
}
