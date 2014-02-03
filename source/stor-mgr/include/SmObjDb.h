#ifndef __SM_OBJ_DB__
#define __SM_OBJ_DB__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include "stor_mgr_err.h"
#include <fds_volume.h>
#include <fds_types.h>
#include "ObjLoc.h"
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <util/Log.h>
#include "StorMgrVolumes.h"
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>

#include <fds_qos.h>
#include <qos_ctrl.h>
#include <fds_obj_cache.h>
#include <fds_assert.h>
#include <fds_config.hpp>
#include <fds_stopwatch.h>

#include <utility>
#include <atomic>
#include <unordered_map>
#include <include/TransJournal.h>
#include <ObjStats.h>
#define SM_TOKEN_MASK 0xff000000;
using namespace FDS_ProtocolInterface;
using namespace fds;
using namespace osm;
using namespace std;
using namespace diskio;

namespace fds {
extern ObjectStorMgr *objStorMgr;
typedef fds_uint32_t fds_token_id;

// TODO:  Fill it up
class SMTokenItr {
 public:
  ObjectID  objId;
  SMTokenItr() {
  }
  ~SMTokenItr() {
  }
};



class SmObjDb { 
 public: 

   SmObjDb(std::string stor_prefix_,
              fds_log* _log) {
     tokDbLog = _log;
     stor_prefix = stor_prefix_;
   }
   ~SmObjDb() { 
   }

   std::unordered_map<fds_token_id, ObjectDB *> tokenTbl;
   fds_log *tokDbLog;
   std::string stor_prefix;

  ObjectDB *openObjectDB(fds_token_id tokId) {
    ObjectDB *objdb = NULL;
     tokId &= SM_TOKEN_MASK;
     if ( (objdb = tokenTbl[tokId]) == NULL ) { 
      // Create leveldb
      std::string filename= stor_prefix + "SNodeObjIndex_" + std::to_string(tokId);
      objdb  = new ObjectDB(filename);
      tokenTbl[tokId] = objdb;
     } 

    return objdb;
  }

void closeObjectDB(fds_token_id tokId) { 
   tokId &= SM_TOKEN_MASK;
   ObjectDB *objdb = NULL;
   if ( (objdb = tokenTbl[tokId]) == NULL ) { 
     return;
   }
   tokenTbl[tokId] = NULL;
   delete objdb;
}


ObjectDB *getObjectDB(fds_token_id tokId) { 
ObjectDB *objdb = NULL;
   tokId &= SM_TOKEN_MASK;
    objdb = tokenTbl[tokId];
    return objdb;
}

fds::Error Get(const ObjectID& obj_id,
                         ObjectBuf& obj_buf) {
  fds_token_id tokId = obj_id.GetHigh() & SM_TOKEN_MASK;
  fds::Error err = ERR_OK;
  ObjectDB *odb = getObjectDB(tokId);
  if (odb) { 
     err =  odb->Get(obj_id, obj_buf);
  } else {
    odb = openObjectDB(tokId);
    err = fds::Error(fds::ERR_DISK_READ_FAILED);
  }
  return err;
}

fds::Error Put(const ObjectID& obj_id,
                         ObjectBuf& obj_buf) {
  fds_token_id tokId = obj_id.GetHigh() & SM_TOKEN_MASK;
  fds::Error err = ERR_OK;
  ObjectDB *odb = getObjectDB(tokId);
  if (odb) { 
     err =  odb->Get(obj_id, obj_buf);
  } else {
    odb = openObjectDB(tokId);
    err = fds::Error(fds::ERR_DISK_WRITE_FAILED);
  }
  return err;
}

fds_int32_t CompareKey(char *key, ObjectID obj_id);
fds_int32_t  RangeCompareKey(ObjectID obj_id, ObjectID start_obj_id, ObjectID end_obj_id) ;
void  iterRetrieveObjects(const fds_token_id &token, 
                                   const size_t &max_size, 
                                   FDSP_MigrateObjectList &obj_list, 
                                   SMTokenItr &itr);
};

}
#endif
