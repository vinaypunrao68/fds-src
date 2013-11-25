#include <cstdarg>
#include <Ice/Ice.h>
#include <fdsp/FDSP.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <Ice/Object.h>
#include <IceUtil/Iterator.h>
#include <IceUtil/CtrlCHandler.h>
#include "StorHvisorNet.h"
//#include "hvisor_lib.h"
#include "StorHvisorCPP.h"
#include <hash/MurmurHash3.h>

void *hvisor_hdl;
StorHvCtrl *storHvisor;

using namespace std;
using namespace FDS_ProtocolInterface;
using namespace Ice;
using namespace IceUtil;

/*
 * Globals being used for the unit test.
 * TODO: This should be cleaned up into
 * unit test specific class. Keeping them
 * global is kinda hackey.
 */
fds_notification notifier;
fds_mutex map_mtx("map mutex");
std::map<fds_uint32_t, std::string> written_data;
std::map<fds_uint32_t, fds_bool_t> verified_data;

static void sh_test_w_callback(void *arg1,
                               void *arg2,
                               fbd_request_t *w_req,
                               int res) {
  fds_verify(res == 0);
  map_mtx.lock();
  /*
   * Copy the write buffer for verification later.
   */
  written_data[w_req->op] = std::string(w_req->buf,
                                        w_req->len);
  verified_data[w_req->op] = false;
  map_mtx.unlock();

  notifier.notify();
  
  delete w_req->buf;
  delete w_req;
}

static void sh_test_nv_callback(void *arg1,
                                void *arg2,
                                fbd_request_t *w_req,
                                int res) {
  fds_verify(res == 0);
  /*
   * Don't cache the write contents. Just
   * notify that the write is complete.
   */
  notifier.notify();
  
  delete w_req->buf;
  delete w_req;
}

static void sh_test_r_callback(void *arg1,
                               void *arg2,
                               fbd_request_t *r_req,
                               int res) {
  fds_verify(res == 0);
  /*
   * Verify the read's contents.
   */  
  map_mtx.lock();
  if (written_data[r_req->op].compare(0,
                                      string::npos,
                                      r_req->buf,
                                      r_req->len) == 0) {
    verified_data[r_req->op] = true;
  } else {
    verified_data[r_req->op] = false;
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::critical) << "FAILED verification of SH test read "
							       << r_req->op;
  }
  map_mtx.unlock();

  notifier.notify();
  
  /*
   * We're not freeing the read buffer here. The read
   * caller owns that buffer and must free it themselves.
   */
  delete r_req;
}

void sh_test_w(const char *data,
               fds_uint32_t len,
               fds_uint32_t offset,
	       fds_volid_t vol_id,
               fds_bool_t w_verify) {
  fbd_request_t *w_req;
  char          *w_buf;
  
  /*
   * Note the buf and request are freed by
   * the callback handler.
   */
  w_req = new fbd_request_t;
  w_req->volUUID = vol_id;
  w_buf = new char[len]();

  /*
   * TODO: We're currently overloading the
   * op field to denote which I/O request
   * this is. The field isn't used, so we can
   * later remove it and use a better way to
   * identify the I/O.
   */
  w_req->op  = offset;
  w_req->buf = w_buf;

  memcpy(w_buf, data, len);

  w_req->sec  = offset;
  w_req->secs = len / HVISOR_SECTOR_SIZE;
  w_req->len  = len;
  w_req->vbd = NULL;
  w_req->vReq = NULL;
  w_req->io_type = 1;
  w_req->hvisorHdl = hvisor_hdl;
  w_req->cb_request = sh_test_w_callback;
  
  if (w_verify == true) {
    pushFbdReq(w_req);
    // pushVolQueue(w_req);
  } else {
    w_req->cb_request = sh_test_nv_callback;
    pushFbdReq(w_req);
    // pushVolQueue(w_req);
  }
  
  /*
   * Wait until we've finished this write
   */
  while (1) {
    notifier.wait_for_notification();
    if (w_verify == true) {
      map_mtx.lock();
      /*
       * Here we're assuming that the offset
       * is increasing by 1 each time.
       */
      if (written_data.size() == offset + 1) {
        map_mtx.unlock();
        break;
      }
      map_mtx.unlock();
    } else {
      break;
    }
  }

  FDS_PLOG(storHvisor->GetLog()) << "Finished SH test write " << offset;
}

int sh_test_r(char *r_buf, fds_uint32_t len, fds_uint32_t offset, fds_volid_t vol_id) {
  fbd_request_t *r_req;
  fds_int32_t    result = 0;

  /*
   * Note the buf and request are freed by
   * the callback handler.
   */
  r_req = new fbd_request_t;
  r_req->volUUID = vol_id;
  /*
   * TODO: We're currently overloading the
   * op field to denote which I/O request
   * this is. The field isn't used, so we can
   * later remove it and use a better way to
   * identify the I/O.
   */
  r_req->op   = offset;
  r_req->buf  = r_buf;

  /*
   * Set the offset to be based
   * on the iteration.
   */
  r_req->sec  = offset;

  /*
   * TODO: Do we need both secs and len? What's
   * the difference?
   */
  r_req->secs = len / HVISOR_SECTOR_SIZE;
  r_req->len  = len;
  r_req->vbd = NULL;
  r_req->vReq = NULL;
  r_req->io_type = 0;
  r_req->hvisorHdl = hvisor_hdl;
  r_req->cb_request = sh_test_r_callback;
  pushFbdReq(r_req);
  // pushVolQueue(r_req);

  /*
   * Wait until we've finished this read
   */
  while (1) {
    notifier.wait_for_notification();
    map_mtx.lock();
    /*
     * Here we're assuming that the offset
     * is increasing by 1 each time.
     */
    if (verified_data[offset] == true) {
      FDS_PLOG(storHvisor->GetLog()) << "Verified SH test read "
                                     << offset;
    } else {
      FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::critical) << "FAILED verification of SH test read "
								 << offset;
      result = -1;
    }
    map_mtx.unlock();
    break;
  }

  return result;
}

int unitTest(fds_uint32_t time_mins) {
  fds_uint32_t req_size;
  char *w_buf;
  char *r_buf;
  fds_uint32_t w_count;
  fds_int32_t result;

  /*
   * Wait before running test for server to be ready.
   * TODO: Remove this and make sure the test isn't
   * kicked off until the server is ready and have server
   * properly wait until it's up.
   */
  sleep(10);

  req_size = 4096;
  w_count  = 5000;
  result   = 0;

  /*
   * Note these buffers are reused at every loop
   * iteration and are freed at the end of the test.
   */
  w_buf    = new char[req_size]();
  r_buf    = new char[req_size]();

  if (time_mins > 0) {
    /*
     * Do a time based unit test.
     */
    boost::xtime stoptime;
    boost::xtime curtime;
    boost::xtime_get(&stoptime, boost::TIME_UTC_);
    stoptime.sec += (60 * time_mins);

    w_count = 0;

    boost::xtime_get(&curtime, boost::TIME_UTC_);
    while (curtime.sec < stoptime.sec) {
      boost::xtime_get(&curtime, boost::TIME_UTC_);

      /*
       * Select a byte string to fill the
       * buffer with.
       * TODO: Let's get more random data
       * than this.
       */
      if (w_count % 3 == 0) {
        memset(w_buf, 0xbeef, req_size);
      } else if (w_count % 2 == 0) {
        memset(w_buf, 0xdead, req_size);
      } else {
        memset(w_buf, 0xfeed, req_size);
      }

      /*
       * Write the data without caching the contents
       * for later verification.
       */
      sh_test_w(w_buf, req_size, w_count, 1, false);
      w_count++;
    }
    
    FDS_PLOG(storHvisor->GetLog()) << "Finished " << w_count
                                   << " writes after " << time_mins
                                   << " minutes";
  } else {
    /*
     * Do a request based unit test
     */
    for (fds_uint32_t i = 0; i < w_count; i++) {
      /*
       * Select a byte string to fill the
       * buffer with.
       * TODO: Let's get more random data
       * than this.
       */
      if (i % 3 == 0) {
        memset(w_buf, 0xbeef, req_size);
      } else if (i % 2 == 0) {
        memset(w_buf, 0xdead, req_size);
      } else {
        memset(w_buf, 0xfeed, req_size);
      }

      sh_test_w(w_buf, req_size, i, 1, true);
    }
    FDS_PLOG(storHvisor->GetLog()) << "Finished all " << w_count
                                   << " test writes";

    for (fds_uint32_t i = 0; i < w_count; i++) {
      memset(r_buf, 0x00, req_size);      
      result = sh_test_r(r_buf, req_size, i, 1);
      if (result != 0) {
        break;
      }
    }
    FDS_PLOG(storHvisor->GetLog()) << "Finished all " << w_count
                                   << " reads";  
  }
  delete w_buf;
  delete r_buf;

  return result;
}

int unitTestFile(const char *inname, const char *outname, unsigned int base_vol_id, int num_volumes) {

  fds_int32_t  result;
  fds_uint32_t req_count;
  fds_uint32_t last_write_len;

  result    = 0;
  req_count = 0;

  /*
   * Wait before running test for server to be ready.
   * TODO: Remove this and make sure the test isn't
   * kicked off until the server is ready and have server
   * properly wait until it's up.
   */
  sleep(10);

  /*
   * Clear any previous test data.
   */
  map_mtx.lock();
  written_data.clear();
  verified_data.clear();
  map_mtx.unlock();

  /*
   * Setup input file
   */
  std::ifstream infile;
  std::string infilename(inname);
  char *file_buf;
  fds_uint32_t buf_len = 4096;
  file_buf = new char[buf_len]();
  std::string line;
  infile.open(infilename, ios::in | ios::binary);

  /*
   * TODO: last_write_len is a hack because
   * the read interface doesn't return how much
   * data was actually read.
   */
  last_write_len = buf_len;

  /*
   * Read all data from input file
   * and write to an object.
   */
  if (infile.is_open()) {
    while (infile.read(file_buf, buf_len)) {
      sh_test_w(file_buf, buf_len, req_count, base_vol_id + (req_count % num_volumes), true);
      req_count++;
    }
    sh_test_w(file_buf, infile.gcount(), req_count, base_vol_id + (req_count % num_volumes), true);
    last_write_len = infile.gcount();
    req_count++;

    infile.close();
  } else {
    return -1;
  }
  
  /*
   * Setup output file
   */
  std::ofstream outfile;
  std::string outfilename(outname);
  outfile.open(outfilename, ios::out | ios::binary);

  /*
   * Issue read and write buffer out to file.
   */
  for (fds_uint32_t i = 0; i < req_count; i++) {
    memset(file_buf, 0x00, buf_len);
    /*
     * TODO: Currently we alway read 4K, even if
     * the buffer is less than that. Eventually,
     * the read should tell us how much was read.
     */
    if (i == (req_count - 1)) {
      buf_len = last_write_len;
    }
    result = sh_test_r(file_buf, buf_len, i, base_vol_id + (i % num_volumes));
    if (result != 0) {
      break;
    }
    outfile.write(file_buf, buf_len);
  }

  outfile.close();

  delete file_buf;

  return result;
}


void CreateStorHvisor(int argc, char *argv[], hv_create_blkdev cr_blkdev, hv_delete_blkdev del_blkdev)
{
  CreateSHMode(argc, argv, cr_blkdev, del_blkdev, false, 0, 0);
}

void CreateStorHvisorS3(int argc, char *argv[])
{
  CreateSHMode(argc, argv, NULL, NULL, false, 0, 0);
}

void CreateSHMode(int argc,
                  char *argv[],
                  hv_create_blkdev cr_blkdev, 
                  hv_delete_blkdev del_blkdev,
                  fds_bool_t test_mode,
                  fds_uint32_t sm_port,
                  fds_uint32_t dm_port)
{
  if (test_mode == true) {
    storHvisor = new StorHvCtrl(argc, argv, StorHvCtrl::TEST_BOTH, sm_port, dm_port);
  } else {
    storHvisor = new StorHvCtrl(argc, argv, StorHvCtrl::NORMAL);
  }

  storHvisor->cr_blkdev = cr_blkdev;
  storHvisor->del_blkdev = del_blkdev;

  /* 
   * Start listening for OM control messages 
   * Appropriate callbacks were setup by data placement and volume table objects  
   */
  storHvisor->StartOmClient();
  // storHvisor->om_client->registerNodeWithOM();
  storHvisor->qos_ctrl->runScheduler();

  FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << "StorHvisorNet - Created storHvisor " << storHvisor;
}

void DeleteStorHvisor()
{
  FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << " StorHvisorNet -  Deleting the StorHvisor";
  delete storHvisor;
}

void ctrlCCallbackHandler(int signal)
{
  FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::notification) << "StorHvisorNet -  Received Ctrl C " << signal;
  storHvisor->_communicator->shutdown();
  DeleteStorHvisor();
}

std::atomic_uint nextIoReqId;

StorHvCtrl::StorHvCtrl(int argc,
                       char *argv[],
                       sh_comm_modes _mode,
                       fds_uint32_t sm_port_num,
                       fds_uint32_t dm_port_num)
  : mode(_mode) {
  std::string  omIpStr;
  fds_uint32_t omConfigPort;
  std::string node_name = "localhost-sh";

  omConfigPort = 0;

  /*
   * Parse out cmdline options here.
   * TODO: We're parsing some options here and
   * some in ubd. We need to unify this.
   */
  for (fds_uint32_t i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--om_ip=", 8) == 0) {
      if (mode == NORMAL) {
        /*
         * Only use the OM's IP in the normal
         * mode. We don't need it in test modes.
         */
        omIpStr = argv[i] + 8;
      }
    } else if (strncmp(argv[i], "--om_port=", 10) == 0) {
      omConfigPort = strtoul(argv[i] + 10, NULL, 0);
    }  else if (strncmp(argv[i], "--node_name=", 12) == 0) {
      node_name = argv[i] + 12;
    } 
    /*
     * We don't complain here about other args because
     * they may have been processed already but not
     * removed from argc/argv
     */
  }
  my_node_name = node_name;
  nextIoReqId = 0;
  

  sh_log = new fds_log("sh", "logs");
  FDS_PLOG(sh_log) << "StorHvisorNet - Constructing the Storage Hvisor";

  /* create OMgr client if in normal mode */
  om_client = NULL;
  FDS_PLOG(sh_log) << "StorHvisorNet - Will create and initialize OMgrClient";

  struct ifaddrs *ifAddrStruct = NULL;
  struct ifaddrs *ifa          = NULL;
  void   *tmpAddrPtr           = NULL;

  /*
   * Get the local IP of the host.
   * This is needed by the OM.
   */
  getifaddrs(&ifAddrStruct);
  for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
      if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
          tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
          char addrBuf[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
          myIp = std::string(addrBuf);
	  if (myIp.find("10.1") != std::string::npos)
	    break; /* TODO: more dynamic */
      }
    }
  }
  assert(myIp.empty() == false);
  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "StorHvisorNet - My IP: " << myIp;
  
  if (ifAddrStruct != NULL) {
    freeifaddrs(ifAddrStruct);
  }

  /*
   * Pass 0 as the data path port since the SH is not
   * listening on that port.
   */
  om_client = new OMgrClient(FDSP_STOR_HVISOR,
                             omIpStr,
                             omConfigPort,
                             myIp,
                             0,
                             node_name,
                             sh_log);
  if (om_client) {
    om_client->initialize();
  }
  else {
    FDS_PLOG_SEV(sh_log, fds::fds_log::error) << "StorHvisorNet - Failed to create OMgrClient, will not receive any OM events";
  }

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("fds.conf");
  _communicator = Ice::initialize(argc, argv, initData);
  Ice::PropertiesPtr props = _communicator->getProperties();

  /*  Create the QOS Controller object */ 
  qos_ctrl = new StorHvQosCtrl(50, fds::FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET, sh_log);
  om_client->registerThrottleCmdHandler(StorHvQosCtrl::throttleCmdHandler);
  qos_ctrl->registerOmClient(om_client); /* so it will start periodically pushing perfstats to OM */

  rpcSwitchTbl = new FDS_RPC_EndPointTbl(_communicator);

  /* TODO: for now StorHvVolumeTable constructor will create 
   * volume 1, revisit this soon when we add multi-volume support
   * in other parts of the system */
  vol_table = new StorHvVolumeTable(this, sh_log);  


  /*
   * Set basic thread properties.
   */
  props->setProperty("StorHvisorClient.ThreadPool.Size", "100");
  props->setProperty("StorHvisorClient.ThreadPool.SizeMax", "200");
  props->setProperty("StorHvisorClient.ThreadPool.SizeWarn", "100");

  FDS_PLOG(sh_log) << "StorHvisorNet - StorHvCtrl basic infra init successfull ";
  
  /*
   * Parse options out of config file
   */

  /*
   * Setup RPC endpoints based on comm mode.
   */
  std::string dataMgrIPAddress;
  int dataMgrPortNum;
  std::string storMgrIPAddress;
  int storMgrPortNum;
  if ((mode == DATA_MGR_TEST) ||
      (mode == TEST_BOTH)) {
    /*
     * If a port_num to use is set use it,
     * otherwise pull from config file.
     */
    if (dm_port_num != 0) {
      dataMgrPortNum = dm_port_num;
    } else {
      dataMgrPortNum = props->getPropertyAsInt("DataMgr.PortNumber");
    }
    dataMgrIPAddress = props->getProperty("DataMgr.IPAddress");
    rpcSwitchTbl->Add_RPC_EndPoint(dataMgrIPAddress, dataMgrPortNum, my_node_name, FDSP_DATA_MGR);
  }
  if ((mode == STOR_MGR_TEST) ||
      (mode == TEST_BOTH)) {
    if (sm_port_num != 0) {
      storMgrPortNum = sm_port_num;
    } else {
      storMgrPortNum  = props->getPropertyAsInt("ObjectStorMgrSvr.PortNumber");
    }
    storMgrIPAddress  = props->getProperty("ObjectStorMgrSvr.IPAddress");
    rpcSwitchTbl->Add_RPC_EndPoint(storMgrIPAddress, storMgrPortNum, my_node_name, FDSP_STOR_MGR);
  }
  
  if ((mode == DATA_MGR_TEST) ||
      (mode == TEST_BOTH)) {
    /*
     * TODO: Currently we always add the DM IP in the DM and BOTH test modes.
     */
    fds_uint32_t ip_num = FDS_RPC_EndPoint::ipString2Addr(dataMgrIPAddress);
    dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NO_OM_MODE,
                                                ip_num,
                                                storMgrPortNum,
                                                dataMgrPortNum,
                                                om_client);
  } else if (mode == STOR_MGR_TEST) {
    fds_uint32_t ip_num = FDS_RPC_EndPoint::ipString2Addr(storMgrIPAddress);
    dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NO_OM_MODE,
                                                ip_num,
                                                storMgrPortNum,
                                                dataMgrPortNum,
                                                om_client);
  } else {
    FDS_PLOG(sh_log) <<"StorHvisorNet -  Entring Normal Data placement mode";
    dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NORMAL_MODE,
                                                om_client);
  }

  /*
   * Only create a ctrl-c handler in normal mode since
   * a test mode may define its own handler (via ICE) or
   * just finish to the end without needing ctrl-c.
   */
#if 0
  if (mode == NORMAL) {
    shCtrlHandler = new IceUtil::CtrlCHandler();
    try {
      shCtrlHandler->setCallback(ctrlCCallbackHandler);
    } catch (const CtrlCHandlerException&) {
      assert(0);
    }
  }
#endif

}

/*
 * Constructor uses comm with DM and SM if no mode provided.
 */
StorHvCtrl::StorHvCtrl(int argc, char *argv[])
    : StorHvCtrl(argc, argv, NORMAL, 0, 0) {

}

StorHvCtrl::StorHvCtrl(int argc,
                       char *argv[],
                       sh_comm_modes _mode)
    : StorHvCtrl(argc, argv, _mode, 0, 0) {

}

StorHvCtrl::~StorHvCtrl()
{
  delete vol_table; 
  delete dataPlacementTbl;
  if (om_client)
    delete om_client;
  delete qos_ctrl;
  delete sh_log;
}

fds::Error StorHvCtrl::pushBlobReq(fds::FdsBlobReq *blobReq) {
  fds_verify(blobReq->magicInUse() == true);
  fds::Error err(ERR_OK);

  /*
   * Pack the blobReq in to a qosReq to pass to QoS
   */
  fds_uint32_t reqId = atomic_fetch_add(&nextIoReqId, (fds_uint32_t)1);
  AmQosReq *qosReq  = new AmQosReq(blobReq, reqId);
  fds_volid_t volId = blobReq->getVolId();

  fds::StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
  if ((shVol == NULL) || (shVol->volQueue == NULL)) {
    shVol->readUnlock();
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::error)
        << "Volume and queueus are NOT setup for volume " << volId;
    err = fds::ERR_INVALID_ARG;
    delete qosReq;
    return err;
  }
  /*
   * TODO: We should handle some sort of success/failure here?
   */
  qos_ctrl->enqueueIO(volId, qosReq);
  shVol->readUnlock();

  FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::debug)
      << "Queued IO for vol " << volId;

  return err;
}

/*
 * TODO: Actually calculate the host's IP
 */
#define SRC_IP           0x0a010a65
#define FDS_IO_LONG_TIME 60 // seconds

fds::Error StorHvCtrl::putBlob(fds::AmQosReq *qosReq) {
  fds::Error err(ERR_OK);

  /*
   * Pull out the blob request
   */
  FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
  fds_verify(blobReq->magicInUse() == true);

  fds_volid_t   volId = blobReq->getVolId();
  StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
  if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
    shVol->readUnlock();
    FDS_PLOG_SEV(sh_log, fds::fds_log::critical) << "putBlob failed to get volume for vol "
                                                 << volId;
    
    blobReq->cbWithResult(-1);
    err = ERR_DISK_WRITE_FAILED;
    delete qosReq;
    return err;
  }

  /*
   * Track how long the request was queued before put() dispatch
   * TODO: Consider moving to the QoS request
   */
  blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
      boost::posix_time::microsec_clock::universal_time()));

  /*
   * Get/lock a journal entry for the request.
   */
  fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                                   blobReq->getBlobOffset());
  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Assigning transaction ID " << transId
                                                   << " to put request";
  StorHvJournalEntry *journEntry = shVol->journal_tbl->get_journal_entry(transId);
  fds_verify(journEntry != NULL);
  StorHvJournalEntryLock jeLock(journEntry);

  /*
   * Check if the entry is already active.
   */
  if (journEntry->isActive() == true) {
    /*
     * There is an on-going transaction for this offset
     * Queue this up for later processing.
     */

    // TODO: For now, just return error :-(
    shVol->readUnlock();
    FDS_PLOG_SEV(sh_log, fds::fds_log::critical) << "Transaction " << transId << " is already ACTIVE"
                                                 << ", just give up and return error.";
    blobReq->cbWithResult(-2);
    err = ERR_DISK_WRITE_FAILED;
    delete qosReq;
    return err;
  }
  journEntry->setActive();

  /*
   * Hash the data to obtain the ID
   */
  ObjectID objId;
  MurmurHash3_x64_128(blobReq->getDataBuf(),
                      blobReq->getDataLen(),
                      0,
                      &objId);
  blobReq->setObjId(objId);

  FDSP_MsgHdrTypePtr msgHdrSm = new FDSP_MsgHdrType;
  FDSP_MsgHdrTypePtr msgHdrDm = new FDSP_MsgHdrType;
  msgHdrSm->glob_volume_id    = volId;
  msgHdrDm->glob_volume_id    = volId;

  /*
   * Setup network put object & catalog request
   */
  FDSP_PutObjTypePtr put_obj_req = new FDSP_PutObjType;
  put_obj_req->data_obj = std::string((const char *)blobReq->getDataBuf(),
                                      (size_t )blobReq->getDataLen());
  put_obj_req->data_obj_len = blobReq->getDataLen();

  FDSP_UpdateCatalogTypePtr upd_obj_req = new FDSP_UpdateCatalogType;
  upd_obj_req->obj_list.clear();

  FDS_ProtocolInterface::FDSP_BlobObjectInfo upd_obj_info;
  upd_obj_info.offset = blobReq->getBlobOffset();  // May need to change to 0 for now?
  upd_obj_info.size = blobReq->getDataLen();

  put_obj_req->data_obj_id.hash_high = upd_obj_info.data_obj_id.hash_high = objId.GetHigh();
  put_obj_req->data_obj_id.hash_low = upd_obj_info.data_obj_id.hash_low = objId.GetLow();

  upd_obj_req->obj_list.push_back(upd_obj_info);
  upd_obj_req->meta_list.clear();
  upd_obj_req->blob_size = blobReq->getDataLen();  // Size of the whole blob? Or just what I'm putting

  

  /*
   * Initialize the journEntry with a open txn
   */
  journEntry->trans_state = FDS_TRANS_OPEN;
  journEntry->io = qosReq;
  journEntry->sm_msg = msgHdrSm;
  journEntry->dm_msg = msgHdrDm;
  journEntry->sm_ack_cnt = 0;
  journEntry->dm_ack_cnt = 0;
  journEntry->dm_commit_cnt = 0;
  journEntry->op = FDS_IO_WRITE;
  journEntry->data_obj_id.hash_high = objId.GetHigh();
  journEntry->data_obj_id.hash_low = objId.GetLow();
  journEntry->data_obj_len = blobReq->getDataLen();

  InitSmMsgHdr(msgHdrSm);
  msgHdrSm->src_ip_lo_addr = SRC_IP;
  msgHdrSm->src_node_name = my_node_name;
  msgHdrSm->req_cookie = transId;

  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Putting object " << objId << " for blob "
                                                   << blobReq->getBlobName() << " and offset "
                                                   << blobReq->getBlobOffset() << " in trans "
                                                   << transId;

  /*
   * Get DLT node list.
   */
  unsigned char dltKey = objId.GetHigh() >> 56;  // TODO: Just pass the objId
  fds_int32_t numNodes = 8;  // TODO: Why 8? Look up vol/blob repl factor
  fds_int32_t nodeIds[numNodes];  // TODO: Doesn't need to be signed
  memset(nodeIds, 0x00, sizeof(fds_int32_t) * numNodes);
  dataPlacementTbl->getDLTNodesForDoidKey(dltKey, nodeIds, &numNodes);
  fds_verify(numNodes > 0);

  /*
   * Issue a put for each SM in the DLT list
   */
  for (fds_uint32_t i = 0; i < numNodes; i++) {
    fds_uint32_t node_ip   = 0;
    fds_uint32_t node_port = 0;
    fds_int32_t node_state = -1;

    dataPlacementTbl->getNodeInfo(nodeIds[i],
                                  &node_ip,
                                  &node_port,
                                  &node_state);
    journEntry->sm_ack[i].ipAddr = node_ip;
    journEntry->sm_ack[i].port   = node_port;
    msgHdrSm->dst_ip_lo_addr     = node_ip;
    msgHdrSm->dst_port           = node_port;
    journEntry->sm_ack[i].ack_status = FDS_CLS_ACK;
    journEntry->num_sm_nodes     = numNodes;

    // Call Put object RPC to SM
    FDS_RPC_EndPoint *endPoint = NULL;
    rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                   node_port,
                                   FDSP_STOR_MGR,
                                   &endPoint);
    fds_verify(endPoint != NULL);

    endPoint->fdspDPAPI->begin_PutObject(msgHdrSm, put_obj_req);
    FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "For transaction " << transId
                                                     << " sent async PUT_OBJ_REQ to SM ip "
                                                     << node_ip << " port " << node_port;
  }

  /*
   * Setup DM messages
   */
  numNodes = 8;  // TODO: Why 8? Use vol/blob repl factor
  InitDmMsgHdr(msgHdrDm);
  upd_obj_req->blob_name = blobReq->getBlobName();
  upd_obj_req->dm_transaction_id  = 1;  // TODO: Don't hard code
  upd_obj_req->dm_operation       = FDS_DMGR_TXN_STATUS_OPEN;
  msgHdrDm->req_cookie     = transId;
  msgHdrDm->src_ip_lo_addr = SRC_IP;
  msgHdrDm->src_node_name  = my_node_name;
  msgHdrDm->src_port       = 0;
  memset(nodeIds, 0x00, sizeof(fds_int32_t) * numNodes);
  dataPlacementTbl->getDMTNodesForVolume(volId, nodeIds, &numNodes);
  fds_verify(numNodes > 0);

  /*
   * Update the catalog for each DMT entry
   */
  for (fds_uint32_t i = 0; i < numNodes; i++) {
    fds_uint32_t node_ip   = 0;
    fds_uint32_t node_port = 0;
    fds_int32_t node_state = -1;
    dataPlacementTbl->getNodeInfo(nodeIds[i],
                                  &node_ip,
                                  &node_port,
                                  &node_state);
    
    journEntry->dm_ack[i].ipAddr        = node_ip;
    journEntry->dm_ack[i].port          = node_port;
    msgHdrDm->dst_ip_lo_addr            = node_ip;
    msgHdrDm->dst_port                  = node_port;
    journEntry->dm_ack[i].ack_status    = FDS_CLS_ACK;
    journEntry->dm_ack[i].commit_status = FDS_CLS_ACK;
    journEntry->num_dm_nodes            = numNodes;
    
    // Call Update Catalog RPC call to DM
    FDS_RPC_EndPoint *endPoint = NULL;
    rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                   node_port,
                                   FDSP_DATA_MGR,
                                   &endPoint);
    fds_verify(endPoint != NULL);

    endPoint->fdspDPAPI->begin_UpdateCatalogObject(msgHdrDm, upd_obj_req);
    FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "For transaction " << transId
                                                     << " sent async UP_CAT_REQ to DM ip "
                                                     << node_ip << " port " << node_port;
  }

  // Schedule a timer here to track the responses and the original request
  IceUtil::Time interval = IceUtil::Time::seconds(FDS_IO_LONG_TIME);
  shVol->journal_tbl->schedule(journEntry->ioTimerTask, interval);

  // Release the vol read lock
  shVol->readUnlock();

  /*
   * Note je_lock destructor will unlock the journal entry automatically
   */
  return err;
}

fds::Error StorHvCtrl::putObjResp(const FDSP_MsgHdrTypePtr& rxMsg,
                                   const FDSP_PutObjTypePtr& putObjRsp) {
  fds::Error  err(ERR_OK);
  fds_int32_t result = 0;
  fds_verify(rxMsg->msg_code == FDSP_MSG_PUT_OBJ_RSP);

  fds_uint32_t transId = rxMsg->req_cookie; 
  fds_volid_t  volId   = rxMsg->glob_volume_id;
  StorHvVolume *vol    =  vol_table->getVolume(volId);
  fds_verify(vol != NULL);  // We should not receive resp for unknown vol

  StorHvVolumeLock vol_lock(vol);
  fds_verify(vol->isValidLocked() == true);  // We should not receive resp for unknown vol

  StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
  fds_verify(txn != NULL);

  StorHvJournalEntryLock je_lock(txn); 

  // Verify transaction is valid and matches cookie from resp message
  fds_verify(txn->isActive() == true);

  result = txn->fds_set_smack_status(rxMsg->src_ip_lo_addr,
                                     rxMsg->src_port);
  fds_verify(result == 0);
  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "For transaction " << transId
                                                   << " recvd PUT_OBJ_RESP from SM ip "
                                                   << rxMsg->src_ip_lo_addr
                                                   << " port " << rxMsg->src_port;
  result = fds_move_wr_req_state_machine(rxMsg);
  fds_verify(result == 0);

  return err;
}

fds::Error StorHvCtrl::upCatResp(const FDSP_MsgHdrTypePtr& rxMsg, 
                                 const FDSP_UpdateCatalogTypePtr& catObjRsp) {
  fds::Error  err(ERR_OK);
  fds_int32_t result = 0;
  fds_verify(rxMsg->msg_code == FDSP_MSG_UPDATE_CAT_OBJ_RSP);

  fds_uint32_t transId = rxMsg->req_cookie;
  fds_volid_t volId    = rxMsg->glob_volume_id;

  StorHvVolume *vol = vol_table->getVolume(volId);
  fds_verify(vol != NULL);

  StorHvVolumeLock vol_lock(vol);
  fds_verify(vol->isValidLocked() == true);

  StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
  fds_verify(txn != NULL);
  StorHvJournalEntryLock je_lock(txn);
  fds_verify(txn->isActive() == true); // Should not get resp for inactive txns
  fds_verify(txn->trans_state != FDS_TRANS_EMPTY);  // Should not get resp for empty txns
  
  if (catObjRsp->dm_operation == FDS_DMGR_TXN_STATUS_OPEN) {
    result = txn->fds_set_dmack_status(rxMsg->src_ip_lo_addr,
                                       rxMsg->src_port);
    fds_verify(result == 0);
    FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Recvd DM TXN_STATUS_OPEN RSP for transId "
                                                     << transId  << " ip " << rxMsg->src_ip_lo_addr
                                                     << " port " << rxMsg->src_port;
  } else {
    fds_verify(catObjRsp->dm_operation == FDS_DMGR_TXN_STATUS_COMMITED);
    result = txn->fds_set_dm_commit_status(rxMsg->src_ip_lo_addr,
                                           rxMsg->src_port);
    fds_verify(result == 0);
    FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Recvd DM TXN_STATUS_COMMITED RSP for transId "
                                                     << transId  << " ip " << rxMsg->src_ip_lo_addr
                                                     << " port " << rxMsg->src_port;
  }  
  result = fds_move_wr_req_state_machine(rxMsg);
  fds_verify(result == 0);

  return err;
}

fds::Error StorHvCtrl::getBlob(fds::AmQosReq *qosReq) {
  fds::Error err(ERR_OK);

  FDS_PLOG_SEV(sh_log, fds::fds_log::normal)
      << "Doing a get blob operation!";

  /*
   * Pull out the blob request
   */
  FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
  fds_verify(blobReq->magicInUse() == true);

  fds_volid_t   volId = blobReq->getVolId();
  StorHvVolume *shVol = vol_table->getVolume(volId);
  if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
    FDS_PLOG_SEV(sh_log, fds::fds_log::critical) << "getBlob failed to get volume for vol "
                                                 << volId;    
    blobReq->cbWithResult(-1);
    err = ERR_DISK_WRITE_FAILED;
    delete qosReq;
    return err;
  }

  /*
   * Track how long the request was queued before get() dispatch
   * TODO: Consider moving to the QoS request
   */
  blobReq->setQueuedUsec(shVol->journal_tbl->microsecSinceCtime(
      boost::posix_time::microsec_clock::universal_time()));

  /*
   * Get/lock a journal entry for the request.
   */
  fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                                   blobReq->getBlobOffset());
  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Assigning transaction ID " << transId
                                                   << " to get request";
  StorHvJournalEntry *journEntry = shVol->journal_tbl->get_journal_entry(transId);
  fds_verify(journEntry != NULL);
  StorHvJournalEntryLock jeLock(journEntry);

  /*
   * Check if the entry is already active.
   */
  if (journEntry->isActive() == true) {
    /*
     * There is an on-going transaction for this offset
     * Queue this up for later processing.
     */

    // TODO: For now, just return error :-(
    FDS_PLOG_SEV(sh_log, fds::fds_log::critical) << "Transaction " << transId << " is already ACTIVE"
                                                 << ", just give up and return error.";
    blobReq->cbWithResult(-2);
    err = ERR_DISK_READ_FAILED;
    delete qosReq;
    return err;
  }
  journEntry->setActive();

  /*
   * Setup msg header
   */
  FDSP_MsgHdrTypePtr msgHdr = new FDSP_MsgHdrType;
  InitSmMsgHdr(msgHdr);
  msgHdr->msg_code       = FDSP_MSG_GET_OBJ_REQ;
  msgHdr->msg_id         =  1;
  msgHdr->req_cookie     = transId;
  msgHdr->glob_volume_id = volId;
  msgHdr->src_ip_lo_addr = SRC_IP;
  msgHdr->src_port       = 0;
  msgHdr->src_node_name  = my_node_name;

  /*
   * Setup journal entry
   */
  journEntry->trans_state = FDS_TRANS_OPEN;
  // journEntry->write_ctx = (void *)req;
  // journEntry->comp_req = comp_req;
  // journEntry->comp_arg1 = arg1; // vbd
  // journEntry->comp_arg2 = arg2; //vreq
  journEntry->sm_msg = msgHdr; 
  journEntry->dm_msg = NULL;
  journEntry->sm_ack_cnt = 0;
  journEntry->dm_ack_cnt = 0;
  journEntry->op = FDS_IO_READ;
  journEntry->data_obj_id.hash_high = 0;
  journEntry->data_obj_id.hash_low = 0;
  journEntry->data_obj_len = blobReq->getDataLen();
  journEntry->io = qosReq;
  journEntry->trans_state = FDS_TRANS_GET_OBJ;

  /*
   * Get the object ID from vcc and add it to journal entry and get msg
   */
  ObjectID objId;
  err = shVol->vol_catalog_cache->Query(blobReq->getBlobName(),
                                        blobReq->getBlobOffset(),
                                        transId,
                                        &objId);
  fds_verify(err == ERR_OK);

  journEntry->data_obj_id.hash_high = objId.GetHigh();
  journEntry->data_obj_id.hash_low  = objId.GetLow();

  FDSP_GetObjTypePtr get_obj_req     = new FDSP_GetObjType;
  get_obj_req->data_obj_id.hash_high = objId.GetHigh();
  get_obj_req->data_obj_id.hash_low  = objId.GetLow();
  get_obj_req->data_obj_len          = blobReq->getDataLen();

  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Getting object " << objId << " for blob "
                                                   << blobReq->getBlobName() << " and offset "
                                                   << blobReq->getBlobOffset() << " in trans "
                                                   << transId;
  /*
   * Look up primary SM from DLT entries
   */
  unsigned char dltKey = objId.GetHigh() >> 56;  // TODO: Just pass the objId
  fds_int32_t numNodes = 8;  // TODO: Why 8? Look up vol/blob repl factor
  fds_int32_t nodeIds[numNodes];  // TODO: Doesn't need to be signed
  dataPlacementTbl->getDLTNodesForDoidKey(dltKey, nodeIds, &numNodes);
  fds_verify(numNodes > 0);

  fds_uint32_t node_ip   = 0;
  fds_uint32_t node_port = 0;
  fds_int32_t node_state = -1;
  /*
   * Get primary SM's node info
   * TODO: We're just assuming it's the first in the list!
   * We should be verifying this somehow.
   */
  dataPlacementTbl->getNodeInfo(nodeIds[0],
                                &node_ip,
                                &node_port,
                                &node_state);
  msgHdr->dst_ip_lo_addr = node_ip;
  msgHdr->dst_port       = node_port;

  FDS_RPC_EndPoint *endPoint = NULL; 
  rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                 node_port,
                                 FDSP_STOR_MGR,
                                 &endPoint);
  fds_verify(endPoint != NULL);

  // RPC getObject to StorMgr
  endPoint->fdspDPAPI->begin_GetObject(msgHdr, get_obj_req);
  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "For trans " << transId
                                                   << " sent async GetObj req to SM";
  
  // Schedule a timer here to track the responses and the original request
  IceUtil::Time interval = IceUtil::Time::seconds(FDS_IO_LONG_TIME);
  shVol->journal_tbl->schedule(journEntry->ioTimerTask, interval);

  /*
   * Note je_lock destructor will unlock the journal entry automatically
   */
  return err;
}

fds::Error StorHvCtrl::getObjResp(const FDSP_MsgHdrTypePtr& rxMsg,
                                  const FDSP_GetObjTypePtr& getObjRsp) {
  fds::Error err(ERR_OK);
  fds_verify(rxMsg->msg_code == FDSP_MSG_GET_OBJ_RSP);

  fds_uint32_t transId = rxMsg->req_cookie;
  fds_volid_t volId    = rxMsg->glob_volume_id;

  StorHvVolume* vol = vol_table->getVolume(volId);
  fds_verify(vol != NULL);  // Should not receive resp for non existant vol

  StorHvVolumeLock vol_lock(vol);
  fds_verify(vol->isValidLocked() == true);

  StorHvJournalEntry *txn = vol->journal_tbl->get_journal_entry(transId);
  fds_verify(txn != NULL);

  StorHvJournalEntryLock je_lock(txn);
  fds_verify(txn->isActive() == true);  // Should not receive resp for inactive txn
  fds_verify(txn->trans_state == FDS_TRANS_GET_OBJ);

  /*
   * how to handle the length miss-match (requested  length and  recived legth from SM differ)?
   * We will have to handle sending more data due to length difference
   */
  //boost::posix_time::ptime ts = boost::posix_time::microsec_clock::universal_time();
  //long lat = vol->journal_tbl->microsecSinceCtime(ts) - req->sh_queued_usec;

  /*
   * Data ready, respond to callback
   */
  fds::AmQosReq   *qosReq  = static_cast<fds::AmQosReq *>(txn->io);
  fds_verify(qosReq != NULL);
  fds::FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
  fds_verify(blobReq != NULL);
  FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "Responding to getBlob trans " << transId
                                                   <<" for blob " << blobReq->getBlobName()
                                                   << " and offset " << blobReq->getBlobOffset()
                                                   << " with result " << rxMsg->result;
  /*
   * Mark the IO complete, clean up txn, and callback
   */
  qos_ctrl->markIODone(txn->io);
  if (rxMsg->result == FDSP_ERR_OK) {
    fds_verify(getObjRsp->data_obj_len == blobReq->getDataLen());
    blobReq->setDataBuf(getObjRsp->data_obj.c_str());
    blobReq->cbWithResult(0);
  } else {
    /*
     * We received an error from SM
     */
    blobReq->cbWithResult(-1);
  }

  txn->reset();
  vol->journal_tbl->releaseTransId(transId);

  /*
   * TODO: We're deleting the request structure. This assumes
   * that the caller got everything they needed when the callback
   * was invoked.
   */
  delete blobReq;

  return err;
}

/*****************************************************************************

*****************************************************************************/
fds::Error StorHvCtrl::deleteBlob(fds::AmQosReq *qosReq) {
  FDS_RPC_EndPoint *endPoint = NULL; 
  unsigned int node_ip = 0;
  fds_uint32_t node_port = 0;
  unsigned int doid_dlt_key=0;
  int num_nodes = 8, i =0;
  int node_ids[8];
  int node_state = -1;
  fds::Error err(ERR_OK);
  ObjectID oid;
  fds_uint32_t vol_id;
  FdsBlobReq *blobReq = qosReq->getBlobReqPtr();
  fds_verify(blobReq->magicInUse() == true);
  DeleteBlobReq *del_blob_req = (DeleteBlobReq *)blobReq;

  fds_volid_t   volId = blobReq->getVolId();
  StorHvVolume *shVol = storHvisor->vol_table->getLockedVolume(volId);
  if ((shVol == NULL) || (shVol->isValidLocked() == false)) {
    shVol->readUnlock();
    FDS_PLOG_SEV(sh_log, fds::fds_log::critical) << "deleteBlob failed to get volume for vol "
                                                 << volId;
    
    blobReq->cbWithResult(-1);
    err = ERR_DISK_WRITE_FAILED;
    delete qosReq;
    return err;
  }

  /* Check if there is an outstanding transaction for this block offset  */
  // fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_block(blobReq->getBlobOffset());
  fds_uint32_t transId = shVol->journal_tbl->get_trans_id_for_blob(blobReq->getBlobName(),
                                                                   blobReq->getBlobOffset());
  StorHvJournalEntry *journEntry = shVol->journal_tbl->get_journal_entry(transId);
  
  StorHvJournalEntryLock je_lock(journEntry);
  
  if (journEntry->isActive()) {
    FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << transId << " - Transaction  is already in ACTIVE state, completing request "
				   << transId << " with ERROR(-2) ";
    // There is an ongoing transaciton for this offset.
    // We should queue this up for later processing once that completes.
    
    // For now, return an error.
    blobReq->cbWithResult(-2);
    err = ERR_INVALID_ARG;
    delete qosReq;
    return err;
  }

  journEntry->setActive();

  FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << transId << " volID:" << volId << " - Activated txn for req :" << transId;
  
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr = new FDSP_MsgHdrType;
  FDS_ProtocolInterface::FDSP_DeleteObjTypePtr del_obj_req = new FDSP_DeleteObjType;
  
  storHvisor->InitSmMsgHdr(fdsp_msg_hdr);
  fdsp_msg_hdr->msg_code = FDSP_MSG_DELETE_OBJ_REQ;
  fdsp_msg_hdr->msg_id =  1;
  
  
  journEntry->trans_state = FDS_TRANS_OPEN;
  journEntry->sm_msg = fdsp_msg_hdr; 
  journEntry->dm_msg = NULL;
  journEntry->sm_ack_cnt = 0;
  journEntry->dm_ack_cnt = 0;
  journEntry->op = FDS_IO_READ;
  journEntry->data_obj_id.hash_high = 0;
  journEntry->data_obj_id.hash_low = 0;
  journEntry->data_obj_len = blobReq->getDataLen();
  journEntry->io = qosReq;
  
  fdsp_msg_hdr->req_cookie = transId;
  
  err = shVol->vol_catalog_cache->Query(blobReq->getBlobName(),
                                        blobReq->getBlobOffset(),
                                        transId,
                                        &oid);
  if (err.GetErrno() == ERR_PENDING_RESP) {
    FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << transId << " volID:" << volId << " - Vol catalog Cache Query pending :" << err.GetErrno() << std::endl ;
    journEntry->trans_state = FDS_TRANS_VCAT_QUERY_PENDING;
    return err.GetErrno();
  }
  
  if (err.GetErrno() == ERR_CAT_QUERY_FAILED)
  {
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << "IO-XID:" << transId << " volID:" << vol_id << " - Error reading the Vol catalog  Error code : " <<  err.GetErrno() << std::endl;
    blobReq->cbWithResult(err.GetErrno());
    return err.GetErrno();
  }
  
  FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << transId << " volID:" << vol_id << " - object ID: " << oid.GetHigh() <<  ":" << oid.GetLow()									 << "  ObjLen:" << journEntry->data_obj_len;
  
  // We have a Cache HIT *$###
  //
  uint64_t doid_dlt = oid.GetHigh();
  doid_dlt_key = (doid_dlt >> 56);
  
  fdsp_msg_hdr->glob_volume_id = vol_id;;
  fdsp_msg_hdr->req_cookie = transId;
  fdsp_msg_hdr->msg_code = FDSP_MSG_DELETE_OBJ_REQ;
  fdsp_msg_hdr->msg_id =  1;
  fdsp_msg_hdr->src_ip_lo_addr = SRC_IP;
  fdsp_msg_hdr->src_port = 0;
  fdsp_msg_hdr->src_node_name = storHvisor->my_node_name;
  del_obj_req->data_obj_id.hash_high = oid.GetHigh();
  del_obj_req->data_obj_id.hash_low = oid.GetLow();
  del_obj_req->data_obj_len = blobReq->getDataLen();
  
  journEntry->op = FDS_IO_READ;
  journEntry->data_obj_id.hash_high = oid.GetHigh();;
  journEntry->data_obj_id.hash_low = oid.GetLow();;
  
  // Lookup the Primary SM node-id/ip-address to send the DeleteObject to
  storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
  if(num_nodes == 0) {
    FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorTx:" << "IO-XID:" << transId << " volID:" << volId << " -  DLT Nodes  NOT  confiigured. Check on OM Manager. Completing request with ERROR(-1)";
    blobReq->cbWithResult(-1);
    return ERR_GET_DLT_FAILED;
  }
  storHvisor->dataPlacementTbl->getNodeInfo(node_ids[0],
                                            &node_ip,
                                            &node_port,
                                            &node_state);
  
  fdsp_msg_hdr->dst_ip_lo_addr = node_ip;
  fdsp_msg_hdr->dst_port       = node_port;
  
  // *****CAVEAT: Modification reqd
  // ******  Need to find out which is the primary SM and send this out to that SM. ********
  storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                             node_port,
                                             FDSP_STOR_MGR,
                                             &endPoint);
  
  // RPC Call DeleteObject to StorMgr
  journEntry->trans_state = FDS_TRANS_DEL_OBJ;
  if (endPoint)
  { 
    endPoint->fdspDPAPI->begin_DeleteObject(fdsp_msg_hdr, del_obj_req);
    FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << "IO-XID:" << transId << " volID:" << volId << " - Sent async DelObj req to SM";
  }
  
  // RPC Call DeleteCatalogObject to DataMgr
  FDS_ProtocolInterface::FDSP_DeleteCatalogTypePtr del_cat_obj_req = new FDSP_DeleteCatalogType;
  num_nodes = 8;
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr_dm = new FDSP_MsgHdrType;
  storHvisor->InitDmMsgHdr(fdsp_msg_hdr_dm);
  fdsp_msg_hdr_dm->msg_code = FDSP_MSG_DELETE_CAT_OBJ_REQ;
  fdsp_msg_hdr_dm->req_cookie = transId;
  fdsp_msg_hdr_dm->src_ip_lo_addr = SRC_IP;
  fdsp_msg_hdr_dm->src_node_name = storHvisor->my_node_name;
  fdsp_msg_hdr_dm->src_port = 0;
  fdsp_msg_hdr_dm->dst_port = node_port;
  storHvisor->dataPlacementTbl->getDMTNodesForVolume(volId, node_ids, &num_nodes);
  
  for (i = 0; i < num_nodes; i++) {
    node_ip = 0;
    node_port = 0;
    node_state = -1;
    storHvisor->dataPlacementTbl->getNodeInfo(node_ids[i],
                                              &node_ip,
                                              &node_port,
                                              &node_state);
    
    journEntry->dm_ack[i].ipAddr = node_ip;
    journEntry->dm_ack[i].port = node_port;
    fdsp_msg_hdr_dm->dst_ip_lo_addr = node_ip;
    fdsp_msg_hdr_dm->dst_port = node_port;
    journEntry->dm_ack[i].ack_status = FDS_CLS_ACK;
    journEntry->dm_ack[i].commit_status = FDS_CLS_ACK;
    journEntry->num_dm_nodes = num_nodes;
    del_cat_obj_req->blob_name = blobReq->getBlobName();
    
    // Call Update Catalog RPC call to DM
    storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                               node_port,
                                               FDSP_DATA_MGR,
                                               &endPoint);
    if (endPoint){
      endPoint->fdspDPAPI->begin_DeleteCatalogObject(fdsp_msg_hdr_dm, del_cat_obj_req);
      FDS_PLOG(storHvisor->GetLog()) << " StorHvisorTx:" << "IO-XID:"
                                     << transId << " volID:" << volId
                                     << " - Sent async DELETE_CAT_OBJ_REQ request to DM at "
                                     <<  node_ip << " port " << node_port;
    }
  }
  // Schedule a timer here to track the responses and the original request
  IceUtil::Time interval = IceUtil::Time::seconds(FDS_IO_LONG_TIME);
  shVol->journal_tbl->schedule(journEntry->ioTimerTask, interval);
  return ERR_OK; // je_lock destructor will unlock the journal entry
}

fds_log* StorHvCtrl::GetLog() {
  return sh_log;
}

void StorHvCtrl::StartOmClient() {

  /* 
   * Start listening for OM control messages 
   * Appropriate callbacks were setup by data placement and volume table objects  
   */
  if (om_client) {
    om_client->startAcceptingControlMessages();
    FDS_PLOG_SEV(sh_log, fds::fds_log::notification) << "StorHvisorNet - Started accepting control messages from OM";
    dInfo = new  FDSP_AnnounceDiskCapability();
    dInfo->disk_iops_max =  10000; /* avarage IOPS */
    dInfo->disk_iops_min =  100; /* avarage IOPS */
    dInfo->disk_capacity = 100;  /* size in GB */
    dInfo->disk_latency_max = 100; /* in milli second */
    dInfo->disk_latency_min = 10; /* in milli second */
    dInfo->ssd_iops_max =  100000; /* avarage IOPS */
    dInfo->ssd_iops_min =  1000; /* avarage IOPS */
    dInfo->ssd_capacity = 100;  /* size in GB */
    dInfo->ssd_latency_max = 100; /* in milli second */
    dInfo->ssd_latency_min = 3; /* in milli second */
    dInfo->disk_type =  FDS_DISK_SATA;
    om_client->registerNodeWithOM(dInfo);
  }

}

BEGIN_C_DECLS
void cppOut( char *format, ... ) {
  va_list argptr;             
 
  va_start( argptr, format );          
  if( *format != '\0' ) {
   	if( *format == 's' ) {
       		char* s = va_arg( argptr, char * );
       		FDS_PLOG(storHvisor->GetLog()) << s;
    	}
  }
  va_end( argptr);
}
END_C_DECLS
