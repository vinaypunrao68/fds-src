/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <fds_process.h>
#include <ObjectId.h>

#include <ObjMeta.h>
#include <object-store/ObjectMetaDb.h>
#include <object-store/ObjectMetadataStore.h>
#include <object-store/ObjectMetaCache.h>

namespace fds {

    class SMChk : public FdsProcess {
    public:
        SMChk(int argc, char *argv[],
                Module **mod_vec) :
                FdsProcess(argc,
                        argv,
                        "platform.conf",
                        "fds.sm.",
                        "smchk.log",
                        mod_vec) {
        }
        ~SMChk() {
        }

        int run() override {
            objectStore->setNumBitsPerToken(16);
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
}  // namespace fds