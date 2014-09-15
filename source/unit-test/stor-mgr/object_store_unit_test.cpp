/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdio>
#include <string>
#include <vector>
#include <bitset>

#include <util/Log.h>
#include <fds_types.h>
#include <fds_module.h>
#include <fds_process.h>
#include <object-store/ObjectStore.h>
#include <concurrency/ThreadPool.h>

namespace fds {

static const fds_uint32_t MAX_TEST_OBJ = 1000;
static const fds_uint32_t MAX_VOLUMES = 50;

static fds_uint32_t keyCounter = 0;

static fds_threadpool pool;

struct TestObject {
    fds_volid_t  volId;
    ObjectID     objId;
    boost::shared_ptr<std::string> value;

    TestObject() :
            volId(keyCounter % MAX_VOLUMES),
            objId(keyCounter++),
            value(new std::string(tmpnam(NULL))) {}
    ~TestObject() {
    }
};

static ObjectStore::unique_ptr objectStore;

static void getObj(TestObject & obj) {
    boost::shared_ptr<std::string> ptr;
    // strCacheManager.get(obj.volId, obj.key, &ptr);
    // GLOGTRACE << "compare " << *obj.value << " = " << *ptr;
    // fds_assert(*ptr == *obj.value);
}

static void addObj(TestObject & obj) {
    GLOGTRACE << "Adding value " << *obj.value << " with id "
              << obj.objId;
    objectStore->putObject(obj.volId, obj.objId, obj.value);
    pool.schedule(getObj, obj, 0);
}

static std::vector<TestObject> testObjs(MAX_TEST_OBJ);

class ObjectStoreTest : public FdsProcess {
  public:
    StorMgrVolumeTable* volTbl;

  public:
    ObjectStoreTest(int argc, char *argv[],
                    Module **mod_vec) :
            FdsProcess(argc,
                       argv,
                       "platform.conf",
                       "fds.sm.",
                       "objstore-test.log",
                       mod_vec) {
        volTbl = new StorMgrVolumeTable();
        objectStore = ObjectStore::unique_ptr(
            new ObjectStore("Unit Test Object Store", volTbl));
    }
    ~ObjectStoreTest() {
    }
    int run() override {
        LOGTRACE << "Starting...";

        fds_volid_t volId = 555;
        VolumeDesc vdesc("objectstore_ut_volume", volId);
        volTbl->registerVolume(vdesc);

        ObjectID objId;
        boost::shared_ptr<const std::string> objData(new std::string("DATA!"));
        objectStore->putObject(volId, objId, objData);

        for (std::vector<TestObject>::iterator i = testObjs.begin();
             testObjs.end() != i;
             ++i) {
            LOGTRACE << "Details volume=" << (*i).volId << " key="
                     << (*i).objId << " value=" << *(*i).value;
            addObj(*i);
        }

        LOGTRACE << "Ending...";
        return 0;
    }
};

static void getObject() {
}

}  // namespace fds

int
main(int argc, char** argv) {
    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        nullptr
    };
    fds::ObjectStoreTest objectStoreTest(argc, argv, smVec);
    return objectStoreTest.main();

    return 0;
}
