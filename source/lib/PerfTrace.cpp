#include "PerfTrace.h"
#include "rdtsc.h"

namespace fds {

class PerfEvent;
using namespace std;
// Per module (SM/DM/SH) perf tracer 
  PerfTracer::PerfTracer() { 
     InitRdtsc();
  }

  PerfTracer::~PerfTracer() { 
  }

 // Start the tracing in the point
  void PerfTracer::TracePointBegin(fds::perfEventType eventId, std::string _ev_name) {
      if ( enabled == false) return;
      if ( perfEventTbl[eventId] ) { 
         if ( perfEventTbl[eventId]->enabled == false ) return;  
         perfEventTbl[eventId]->start_cycle = RDTSC();
         perfEventTbl[eventId]->event_count++;
      } else {
        perfEventTbl[eventId] = new PerfEvent(eventId, _ev_name); 
        perfEventTbl[eventId]->start_cycle = RDTSC();
        perfEventTbl[eventId]->event_count++;
      }
 }


  // Stop the tracing in the point
  void PerfTracer::TracePointEnd(fds::perfEventType eventId) {
      if ( enabled == false) return;
      if ( perfEventTbl[eventId] ) { 
         if ( perfEventTbl[eventId]->enabled == false ) return;  
          perfEventTbl[eventId]->end_cycle = RDTSC();
          perfEventTbl[eventId]->tot_cycle = RDTSC() - perfEventTbl[eventId]->start_cycle;
      }
  }

 // Trace Point Stats Increment for this eventId
  void PerfTracer::TracePointStatsInc(fds::perfEventType eventId) {
     if ( enabled == false) return;
     if ( perfEventTbl[eventId] ) {
         if ( perfEventTbl[eventId]->enabled == false ) return;  
          perfEventTbl[eventId]->event_count++;
     }
  }

// Start Performance trace for the module
  void PerfTracer::startPerfTrace() {
    enabled = true;
  }
// Stop Performance trace for the module
  void PerfTracer::stopPerfTrace() {
    enabled = false;

  }
}
