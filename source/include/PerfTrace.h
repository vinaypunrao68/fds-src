#ifndef SOURCE_PERF_TRACE_H_
#define SOURCE_PERF_TRACE_H_

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
#include <fdsp/FDSP.h>
#include <fds_volume.h>
#include <fds_types.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <Ice/Ice.h>
#include <util/Log.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <concurrency/RwLock.h>


#include <utility>
#include <atomic>
#include <list>
#include <unordered_map>
#include "rdtsc.h"

namespace fds { 
using namespace std;

    typedef enum {
      TRACE_SH_QOS_ENQUEUE,
      TRACE_SH_QOS_DEQUEUE,
      TRACE_SH_MURMUR3_HASH,
      TRACE_SH_DLT_LKUP,
      TRACE_SH_DMT_LKUP,
      TRACE_SH_PUT_OBJ_REQ,
      TRACE_SH_PUT_OBJ_RESP,
      TRACE_SH_GET_OBJ_REQ,
      TRACE_SH_GET_OBJ_RESP,

    
      TRACE_SM_QOS_ENQUEUE,
      TRACE_SM_QOS_DEQUEUE,
      TRACE_SM_PUT_OBJ_DEDUPE_CHK,
      TRACE_SM_PERSIST_DISK_WRITE,
      TRACE_SM_PUT_OBJ_LOC_INDX_UPDATE,

      TRACE_SM_GET_OBJ_CACHE_LKUP,
      TRACE_SM_GET_OBJ_LKUP_LOC_INDX,
      TRACE_SM_GET_OBJ_PL_READ_DISK,
    } perfEventType;

    class PerfEvent { 
      public:
  
       perfEventType   eventId;
       std::string          eventName;
       fds_bool_t           enabled;
       fds_uint64_t         start_cycle;
       fds_uint64_t         end_cycle;
       fds_uint64_t         tot_cycle;
       fds_uint64_t         event_count;

       PerfEvent(perfEventType Id, std::string _name) ;
       ~PerfEvent();
    };


   // Per module (SM/DM/SH) perf tracer 
    class PerfTracer {
      public: 

      typedef std::unordered_map<fds_uint32_t, PerfEvent*> perfEventTblType;
      perfEventTblType perfEventTbl;
      fds_rwlock ptrace_lock;
      fds_bool_t  enabled;

      PerfTracer();

      ~PerfTracer();

      // Start the tracing in the point
      void TracePointBegin(perfEventType eventId, std::string _evname);
      // Stop the tracing in the point
      void TracePointEnd(perfEventType eventId);
      // Trace Point Stats Increment for this eventId
      void TracePointStatsInc(perfEventType eventId);

      // Start Performance trace for the module
      void startPerfTrace();
     // Stop Performance trace for the module
      void stopPerfTrace();
  };

}
#endif
