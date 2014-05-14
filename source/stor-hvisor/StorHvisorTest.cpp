#include <iostream>
#include <chrono>
#include <cstdarg>
#include <fds_module.h>
#include <fds_timer.h>
#include "StorHvisorNet.h"
#include "StorHvisorCPP.h"
#include "hvisor_lib.h"
#include <hash/MurmurHash3.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NetSession.h"


void *hvisor_hdl;
extern StorHvCtrl *storHvisor;

using namespace std;
using namespace FDS_ProtocolInterface;

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


static int sh_test_put_callback(void* context, fds_uint64_t buf_size, fds_off_t offset,
                                char* buf, void *callback_data,
				FDSN_Status status, ErrorDetails* errDetaills)
{
  FDS_PLOG(storHvisor->GetLog()) << "sh_test_put_callback is called with status " << status;
  return 0;
}

static FDSN_Status sh_test_get_callback(void* context, fds_uint64_t buf_size, const char* buf, void *callback_data,
				FDSN_Status status, ErrorDetails* errDetaills)
{
  FDS_PLOG(storHvisor->GetLog()) << "sh_test_get_callback is called with status " << status
				 << " data length " << buf_size;
  return status;
}

static void sh_test_delete_callback(FDSN_Status status, const ErrorDetails* errDetails, void* callback_data)
{
  FDS_PLOG(storHvisor->GetLog()) << "sh_test_delete_callback is called with status " << status;
}

static void sh_test_create_bucket_callback(FDSN_Status status, const ErrorDetails* errDetails, void* callback_data)
{
  FDS_PLOG(storHvisor->GetLog()) << "sh_test_create_bucket_callback is called with status " << status;
}

static void sh_test_list_bucket_callback(int isTruncated, const char* nextMarker, int count, const ListBucketContents* contents,
					 int comminPrefixesCount, const char**commonPrefixes, void* callback_data, FDSN_Status status)
{
  FDS_PLOG(storHvisor->GetLog()) << "sh_test_list_bucket_callback is called with status " << status; 
  if (contents == NULL) {
    return;
  }
  for (int i = 0; i < count; ++i) {
    FDS_PLOG(storHvisor->GetLog()) << "content #" << i
				   << " key " << contents[i].objKey
				   << " size " << contents[i].size;
  }
}

static void sh_test_modify_bucket_callback(FDSN_Status status, const ErrorDetails* errDetails, void* callback_data)
{
  FDS_PLOG(storHvisor->GetLog()) << "sh_modify_bucket_callback is called with status " << status;
}

static void sh_test_get_bucket_stats_callback(const std::string& timestamp, int content_count, const BucketStatsContent* contents,
					      void *req_context, void *callback_data, 
					      FDSN_Status status, ErrorDetails* errDetails)
{
  FDS_PLOG(storHvisor->GetLog()) << "sh_get_bucket_stats_callback is called with status " << status;
  if (contents == NULL)
    return;
 
  for (int i = 0; i < content_count; ++i) {
    FDS_PLOG(storHvisor->GetLog()) << "content #" << i
				   << " name " << contents[i].bucket_name
				   << " prio " << contents[i].priority
				   << " performance " << contents[i].performance
				   << " sla " << contents[i].sla
				   << " limit " << contents[i].limit;
  }
}


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

// TEMP -- for testing native api directly, not called from 
// desktop unit test, can exchange names with unitTest and it will be
// called with desktop test
int unitTest2(fds_uint32_t time_mins)
{
  fds_uint32_t req_size;
  char *w_buf;
  char *r_buf;
  fds_uint32_t result;
  FDS_NativeAPI* api;
  BucketContext* buck_context;
  PutPropertiesPtr put_props;
  GetConditions get_conds;

  VolumeDesc voldesc("default_vol2", 2);
  voldesc.iops_min = 10;
  voldesc.iops_max = 1000;

  QosParams qos_params(386, 943, 8);

  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- testing putObject()";

  req_size = 8192;
  result = 0;
  /*
   * Note these buffers are reused at every loop
   * iteration and are freed at the end of the test.
   */
  w_buf    = new char[req_size]();
  r_buf    = new char[2*req_size]();

  /* do one request for now */
  api = new FDS_NativeAPI(FDS_NativeAPI::FDSN_AWS_S3);
  buck_context = new BucketContext("hostA", "default_vol2", "X", "Y");
  put_props.reset(new PutProperties());

  /* first create bucket */
  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- will create bucket " << buck_context->bucketName;
  api->CreateBucket(buck_context, CannedAclPublicRead, NULL, sh_test_create_bucket_callback, NULL);
  sleep(5);

  /* modify bucket's qos params*/
  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- will modify qos params for bucket " << buck_context->bucketName;
  api->ModifyBucket(buck_context, qos_params, NULL, sh_test_modify_bucket_callback, NULL);
  sleep(5);

  // Create a fake transaction for now
  BlobTxId::ptr txDesc(new BlobTxId());

  /* test put requests to bucket */
  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- will put object to " << buck_context->bucketName;
  memset(w_buf, 0xfeed, req_size);
  api->PutBlob(buck_context, "ut_key", put_props, NULL, w_buf, 0, req_size,
               txDesc, false, sh_test_put_callback, NULL); 

  sleep(1);
  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- will put object to " << buck_context->bucketName;
  memset(w_buf, 0xfe0d, req_size);
  api->PutBlob(buck_context, "another_test_key", put_props, NULL, w_buf, 0, req_size,
               txDesc, false, sh_test_put_callback, NULL);

  sleep(2);
  FDS_PLOG(storHvisor->GetLog()) << "Blob write unit test -- waited 2 sec after putObject()";

  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- will get stats of all buckets ";
  api->GetBucketStats(NULL, sh_test_get_bucket_stats_callback, NULL);
  sleep(3);

  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- will get bucket list from bucket " << buck_context->bucketName;
  api->GetBucket(buck_context, "", "", "", 10, NULL, sh_test_list_bucket_callback, NULL);
  sleep(5);

  /*
  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- will get same object from " << buck_context->bucketName;
  api->GetObject(buck_context, "ut_key", &get_conds, 0, 0, r_buf, 2*req_size, NULL, sh_test_get_callback, NULL);
  sleep(5);
  */

  FDS_PLOG(storHvisor->GetLog()) << "Blob unit test -- will delete same object from " << buck_context->bucketName;
  api->DeleteObject(buck_context, "ut_key", NULL, sh_test_delete_callback, NULL);
  sleep(10);

  delete buck_context;
  delete api;

  delete w_buf;
  delete r_buf;

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

