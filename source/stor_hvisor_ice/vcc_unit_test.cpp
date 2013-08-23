/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stor_hvisor_ice/VolumeCatalogCache.h>
#include <stor_hvisor_ice/StorHvisorNet.h>

#include <iostream>  // NOLINT(*)
#include <vector>
#include <string>
#include <list>

#include "util/Log.h"

/*
 * Piggyback on the global storHvisor that's been
 * declared elsewhere.
 */
extern StorHvCtrl *storHvisor;

namespace fds {

class VccUnitTest {
 private:
  std::list<std::string>  unit_tests;

  fds_log *vcc_log;

  /*
   * Unit test funtions
   */
  int basic_update() {
    Error err(ERR_OK);

    return 0;
  }

  int basic_query() {
    Error err(ERR_OK);

    return 0;
  }

  int basic_uq() {
    Error err(ERR_OK);

    return 0;
  }

  int basic_sh_integration() {
    Error err(ERR_OK);
    
    VolumeCatalogCache vcc(storHvisor,
                           vcc_log);
    fds_uint64_t vol_uuid;

    vol_uuid = 987654321;

    err = vcc.RegVolume(vol_uuid);
    if (!err.ok()) {
      std::cout << "Failed to register volume " << vol_uuid << std::endl;
      return -1;
    }

    for (fds_uint32_t i = 0; i < 100; i++) {
      fds_uint64_t block_id = 1 + i;
      ObjectID oid;
      err = vcc.Query(vol_uuid, block_id, &oid);
      if (!err.ok() && err != ERR_PENDING_RESP) {
        std::cout << "Failed to query volume " << vol_uuid << std::endl;
        return -1;
      }
    }

    delete storHvisor;
    
    return 0;
  }

 public:
  VccUnitTest() {
    vcc_log = new fds_log("vcc", "logs");

    unit_tests.push_back("basic_update");
    unit_tests.push_back("basic_query");
    unit_tests.push_back("basic_uq");
    unit_tests.push_back("basic_sh_integration");
    
    /*
     * Create the SH control. Pass some empty cmdline args.
     */
    int argc = 0;
    char* argv[argc];
    storHvisor = new StorHvCtrl(argc, argv, StorHvCtrl::DATA_MGR_TEST);
  }

  ~VccUnitTest() {
    delete vcc_log;
  }

  void Run(const std::string& testname) {
    int result;
    std::cout << "Running unit test \"" << testname << "\"" << std::endl;

    if (testname == "basic_update") {
      result = basic_update();
    } else if (testname == "basic_query") {
      result = basic_query();
    } else if (testname == "basic_uq") {
      result = basic_uq();
    } else if (testname == "basic_sh_integration") {
      result = basic_sh_integration();
    } else {
      std::cout << "Unknown unit test " << testname << std::endl;
    }

    if (result == 0) {
      std::cout << "Unit test \"" << testname << "\" PASSED"  << std::endl;
    } else {
      std::cout << "Unit test \"" << testname << "\" FAILED" << std::endl;
    }
    std::cout << std::endl;
  }

  void Run() {
    for (std::list<std::string>::iterator it = unit_tests.begin();
         it != unit_tests.end();
         ++it) {
      Run(*it);
    }
  }
};

/*
 * Ice communication classes
 */
class ShClientCb : public FDS_ProtocolInterface::FDSP_DataPathResp {
 public:
  void GetObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&,
                     const FDS_ProtocolInterface::FDSP_GetObjTypePtr&,
                     const Ice::Current&) {
  }

  void PutObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&,
                     const FDS_ProtocolInterface::FDSP_PutObjTypePtr&,
                     const Ice::Current&) {
  }

  void UpdateCatalogObjectResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                               fdsp_msg,
                               const
                               FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr&
                               cat_obj_req,
                               const Ice::Current &) {
    std::cout << "Got a update catalog response!" << std::endl;
  }

  void QueryCatalogObjectResp(const
                              FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                              fdsp_msg,
                               const
                              FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr&
                              cat_obj_req,
                              const Ice::Current &) {
    ObjectID oid(cat_obj_req->data_obj_id.hash_high,
                 cat_obj_req->data_obj_id.hash_high);
    std::cout << "Got a response query catalog for object " << oid << std::endl;
  }

  void OffsetWriteObjectResp(const
                             FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                             fdsp_msg,
                             const
                             FDS_ProtocolInterface::FDSP_OffsetWriteObjTypePtr&
                             offset_write_obj_req,
                             const Ice::Current &) {
  }
  void RedirReadObjectResp(const
                           FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                           fdsp_msg,
                           const
                           FDS_ProtocolInterface::FDSP_RedirReadObjTypePtr&
                           redir_write_obj_req,
                           const Ice::Current &) {
  }
};

class ShClient : public Ice::Application {
 public:
  ShClient() {
  }
  ~ShClient() {
  }

  /*
   * Ice will execute the application via run()
   */
  int run(int argc, char* argv[]) {
    /*
     * Process the cmdline args.
     */
    std::string testname;
    for (int i = 1; i < argc; i++) {
      if (strncmp(argv[i], "--testname=", 11) == 0) {
        testname = argv[i] + 11;
      } else {
        std::cout << "Invalid argument " << argv[i] << std::endl;
        return -1;
      }
    }

    /*
     * Setup the basic unit test.
     */
    VccUnitTest unittest;

    if (testname.empty()) {
      unittest.Run();
    } else {
      unittest.Run(testname);
    }

    return 0;
  }

 private:
};

}  // namespace fds

int main(int argc, char* argv[]) {
  fds::ShClient sh_client;

  return sh_client.main(argc, argv, "fds.conf");
}
