#include <cstdarg>
#include <Ice/Ice.h>
#include <FDSP.h>
#include <ObjectFactory.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <Ice/Object.h>
#include <IceUtil/Iterator.h>
#include <IceUtil/CtrlCHandler.h>
#include "StorHvisorNet.h"
//#include "hvisor_lib.h"
#include "StorHvisorCPP.h"

StorHvCtrl *storHvisor;

using namespace std;
using namespace FDS_ProtocolInterface;
using namespace Ice;
using namespace IceUtil;

struct fbd_device *fbd_dev;
extern vvc_vhdl_t vvc_vol_create(volid_t vol_id, const char *db_name, int max_blocks);
int  runningFlag = 1;

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

static void sh_test_r_callback(void *arg1,
                               void *arg2,
                               fbd_request_t *r_req,
                               int res) {
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
    FDS_PLOG(storHvisor->GetLog()) << "FAILED verification of SH test read "
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

void sh_test_w(const char *data, fds_uint32_t len, fds_uint32_t offset) {
  fbd_request_t *w_req;
  char          *w_buf;
  
  /*
   * Note the buf and request are freed by
   * the callback handler.
   */
  w_req = new fbd_request_t;
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
  
  StorHvisorProcIoWr(NULL,
                     w_req,
                     sh_test_w_callback,
                     NULL,
                     NULL);
  
  /*
   * Wait until we've finished this write
   */
  while (1) {
    notifier.wait_for_notification();
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
  }

  FDS_PLOG(storHvisor->GetLog()) << "Finished SH test write " << offset;
}

int sh_test_r(char *r_buf, fds_uint32_t len, fds_uint32_t offset) {
  fbd_request_t *r_req;
  fds_int32_t    result = 0;

  /*
   * Note the buf and request are freed by
   * the callback handler.
   */
  r_req = new fbd_request_t;
  
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
  
  StorHvisorProcIoRd(NULL,
                     r_req,
                     sh_test_r_callback,
                     NULL,
                     NULL);

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
      FDS_PLOG(storHvisor->GetLog()) << "FAILED verification of SH test read "
                                     << offset;
      result = -1;
    }
    map_mtx.unlock();
    break;
  }

  return result;
}

int unitTest() {
  fbd_request_t *w_req;
  fbd_request_t *r_req;
  fds_uint32_t req_size;
  char *w_buf;
  char *r_buf;
  fds_uint32_t w_count;
  fds_int32_t result = 0;

  req_size = 4096;
  w_count  = 5000;

  for (fds_uint32_t i = 0; i < w_count; i++) {
    /*
     * Note the buf and request are freed by
     * the callback handler.
     */
    w_req = new fbd_request_t;
    w_buf = new char[req_size]();

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
    
    /*
     * TODO: We're currently overloading the
     * op field to denote which I/O request
     * this is. The field isn't used, so we can
     * later remove it and use a better way to
     * identify the I/O.
     */
    w_req->op   = i;
    w_req->buf  = w_buf;
    /*
     * Set the offset to be based
     * on the iteration.
     */
    w_req->sec  = i;
    w_req->secs = req_size / HVISOR_SECTOR_SIZE;
    w_req->len  = req_size;
    
    StorHvisorProcIoWr(NULL,
                       w_req,
                       sh_test_w_callback,
                       NULL,
                       NULL);

    /*
     * Wait until we've finished this write
     */
    while (1) {
      notifier.wait_for_notification();
      map_mtx.lock();
      if (written_data.size() == i + 1) {
        map_mtx.unlock();
        break;
      }
      map_mtx.unlock();
    }
    FDS_PLOG(storHvisor->GetLog()) << "Finished SH test write " << i;
  }
  FDS_PLOG(storHvisor->GetLog()) << "Finished all " << w_count
                                 << " test writes";

  r_buf = new char[req_size]();
  for (fds_uint32_t i = 0; i < w_count; i++) {
    /*
     * Note the buf and request are freed by
     * the callback handler.
     */
    r_req = new fbd_request_t;
    memset(r_buf, 0x00, req_size);

    /*
     * TODO: We're currently overloading the
     * op field to denote which I/O request
     * this is. The field isn't used, so we can
     * later remove it and use a better way to
     * identify the I/O.
     */
    r_req->op   = i;
    r_req->buf  = r_buf;
    /*
     * Set the offset to be based
     * on the iteration.
     */
    r_req->sec  = i;
    /*
     * TODO: Do we need both secs and len? What's
     * the difference?
     */
    r_req->secs = req_size / HVISOR_SECTOR_SIZE;
    r_req->len  = req_size;
    
    StorHvisorProcIoRd(NULL,
                       r_req,
                       sh_test_r_callback,
                       NULL,
                       NULL);

    /*
     * Wait until we've finished this read
     */
    while (1) {
      notifier.wait_for_notification();
      map_mtx.lock();
      if (verified_data[i] == true) {
        FDS_PLOG(storHvisor->GetLog()) << "Verified SH test read "
                                       << i;
      } else {
        FDS_PLOG(storHvisor->GetLog()) << "FAILED verification of SH test read "
                                       << i;
        result = -1;
      }
      map_mtx.unlock();
      break;
    }

    if (result != 0) {
      break;
    }
  }
  FDS_PLOG(storHvisor->GetLog()) << "Finished all " << w_count
                                 << " reads";  
  delete r_buf;
  return result;
}

int unitTestFile(const char *inname, const char *outname) {

  fds_int32_t  result;
  fds_uint32_t req_count;
  fds_uint32_t last_write_len;

  result    = 0;
  req_count = 0;

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
      sh_test_w(file_buf, buf_len, req_count);
      req_count++;
    }
    sh_test_w(file_buf, infile.gcount(), req_count);
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
    result = sh_test_r(file_buf, buf_len, i);
    if (result != 0) {
      break;
    }
    outfile.write(file_buf, buf_len);
  }

  outfile.close();

  delete file_buf;

  return result;
}


void CreateStorHvisor(int argc, char *argv[])
{
  CreateSHMode(argc, argv, false, 0, 0);
}

void CreateSHMode(int argc,
                  char *argv[],
                  fds_bool_t test_mode,
                  fds_uint32_t sm_port,
                  fds_uint32_t dm_port)
{
  if (test_mode == true) {
    storHvisor = new StorHvCtrl(argc, argv, StorHvCtrl::TEST_BOTH, sm_port, dm_port);
  } else {
    storHvisor = new StorHvCtrl(argc, argv, StorHvCtrl::NORMAL);
  }

  FDS_PLOG(storHvisor->GetLog()) << "StorHvisorNet - Created storHvisor " << storHvisor
            << " with journal table " << storHvisor->journalTbl;

}

void DeleteStorHvisor()
{
      FDS_PLOG(storHvisor->GetLog()) <<" StorHvisorNet -  Deleting the StorHvisor";
        delete storHvisor;
}

void ctrlCCallbackHandler(int signal)
{
  FDS_PLOG(storHvisor->GetLog()) << "StorHvisorNet -  Received Ctrl C " << signal;
  runningFlag = 0;
  storHvisor->_communicator->shutdown();
  DeleteStorHvisor();
}

StorHvCtrl::StorHvCtrl(int argc,
                       char *argv[],
                       sh_comm_modes _mode,
                       fds_uint32_t sm_port_num,
                       fds_uint32_t dm_port_num)
    : mode(_mode) {
  

  sh_log = new fds_log("sh", "logs");
  FDS_PLOG(sh_log) << "StorHvisorNet - Constructing the Storage Hvisor";

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("fds.conf");
  _communicator = Ice::initialize(argc, argv, initData);
  Ice::PropertiesPtr props = _communicator->getProperties();

  rpcSwitchTbl = new FDS_RPC_EndPointTbl(_communicator);
  journalTbl = new StorHvJournal(FDS_READ_WRITE_LOG_ENTRIES);
  volCatalogCache = new VolumeCatalogCache(this,sh_log);
  
  /*
   * Set basic thread properties.
   */
  props->setProperty("StorHvisorClient.ThreadPool.Size", "10");
  props->setProperty("StorHvisorClient.ThreadPool.SizeMax", "20");
  props->setProperty("StorHvisorClient.ThreadPool.SizeWarn", "18");

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
      (mode == TEST_BOTH) ||
      (mode == NORMAL)) {
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
    rpcSwitchTbl->Add_RPC_EndPoint(dataMgrIPAddress, dataMgrPortNum, FDSP_DATA_MGR);
  }
  if ((mode == STOR_MGR_TEST) ||
      (mode == TEST_BOTH) ||
      (mode == NORMAL)) {
    if (sm_port_num != 0) {
      storMgrPortNum = sm_port_num;
    } else {
      storMgrPortNum  = props->getPropertyAsInt("ObjectStorMgrSvr.PortNumber");
    }
    storMgrIPAddress  = props->getProperty("ObjectStorMgrSvr.IPAddress");
    rpcSwitchTbl->Add_RPC_EndPoint(storMgrIPAddress, storMgrPortNum, FDSP_STOR_MGR);
  }
  
  if ((mode == DATA_MGR_TEST) ||
      (mode == TEST_BOTH)) {
    /*
     * TODO: Currently we always add the DM IP in the DM and BOTH test modes.
     */
    fds_uint32_t ip_num = FDS_RPC_EndPoint::ipString2Addr(dataMgrIPAddress);
    dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NO_OM_MODE,
                                                ip_num);
  } else if (mode == STOR_MGR_TEST) {
    fds_uint32_t ip_num = FDS_RPC_EndPoint::ipString2Addr(storMgrIPAddress);
    dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NO_OM_MODE,
                                                ip_num);
  } else {
    FDS_PLOG(sh_log) <<"StorHvisorNet -  Entring Normal Data placement mode";
    dataPlacementTbl  = new StorHvDataPlacement(StorHvDataPlacement::DP_NORMAL_MODE);
  }

  /*
   * Only create a ctrl-c handler in normal mode since
   * a test mode may define its own handler (via ICE) or
   * just finish to the end without needing ctrl-c.
   */
  if (mode == NORMAL) {
    shCtrlHandler = new IceUtil::CtrlCHandler();
    try {
      shCtrlHandler->setCallback(ctrlCCallbackHandler);
    } catch (const CtrlCHandlerException&) {
      assert(0);
    }
  }

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
  delete sh_log;
//  delete journalTbl;
//  delete volCatalogCache;
//  delete dataPlacementTbl;
}


fds_log* StorHvCtrl::GetLog() {
  return sh_log;
}

BEGIN_C_DECLS
void *hvisor_lib_init(void)
{
  // int err = -ENOMEM;
//        int rc;

        fbd_dev = (fbd_device *)malloc(sizeof(*fbd_dev));
        if (!fbd_dev)
          return (void *)-ENOMEM;
        memset(fbd_dev, 0, sizeof(*fbd_dev));

        fbd_dev->blocksize = 4096; /* default value  */
        fbd_dev->bytesize = 0;
        /* we will have to  generate the dev id for multiple device support */
        fbd_dev->dev_id = 0;

        fbd_dev->vol_id = 1; /*  this should be comming from OM */
#if 0
        if((fbd_dev->vhdl = vvc_vol_create(fbd_dev->vol_id, NULL,8192)) == 0 )
        {
                printf(" Error: creating the  vvc  volume \n");
                goto out;
        }


        rc = pthread_create(&fbd_dev->rx_thread, NULL, fds_rx_io_proc, fbd_dev);
        if (rc) {
          printf(" Error: creating the   RX IO thread : %d \n", rc);
                goto out;
        }
#endif

        return (fbd_dev);
        // return (void *)err;
}

END_C_DECLS


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
