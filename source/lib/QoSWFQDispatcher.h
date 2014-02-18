#include "fds_qos.h"
#include <map>
#include <atomic>

namespace fds {

  const unsigned int MAX_PENDING_IOS_PER_VOLUME = 1024;

  const unsigned int io_dispatch_type_rate = 0;
  const unsigned int io_dispatch_type_credit = 1;
  const unsigned int io_dispatch_type_priority = 2;

  class WFQQueueDesc {

  public:

    fds_uint32_t queue_id;
    fds_uint64_t queue_rate;
    fds_uint32_t queue_priority;
    fds_uint32_t rate_based_weight;
    fds_uint32_t priority_based_weight;

    std::vector<fds_uint64_t> rate_based_rr_spots;
    fds_uint32_t num_priority_based_ios_dispatched; // number of ios dispatched in the current round of priority based WFQ;
    fds_uint32_t num_rate_based_credits;
    fds_uint32_t max_rate_based_credits;

    FDS_VolumeQueue *queue;
    std::atomic<unsigned int> num_pending_ios;
    std::atomic<unsigned int> num_outstanding_ios;

    WFQQueueDesc(fds_uint32_t q_id,
		 FDS_VolumeQueue *que,
		 fds_uint64_t q_rate,
		 fds_uint32_t q_pri)
      : queue_id(q_id),
	queue(que),
	queue_rate(q_rate),
	queue_priority(q_pri) {
      rate_based_rr_spots.clear();
      num_pending_ios = ATOMIC_VAR_INIT(0);
      num_outstanding_ios = ATOMIC_VAR_INIT(0);
      num_rate_based_credits = 0;
      max_rate_based_credits = 0;
    }

  };

  class QoSWFQDispatcher : public FDS_QoSDispatcher {

  private:

    fds_uint64_t total_capacity;
    int num_queues;
    std::map<fds_uint32_t, WFQQueueDesc *> queue_desc_map;
    std::vector<fds_uint32_t> rate_based_qlist;
    fds_uint64_t next_rate_based_spot;
    fds_uint64_t total_rate_based_spots; 
    fds_uint32_t next_priority_based_queue;

    fds_uint64_t num_ios_dispatched;
    fds_uint64_t num_rate_based_slots_serviced;
    boost::posix_time::ptime last_reset_time;

    fds_uint32_t priority_to_wfq_weight(fds_uint32_t priority) {
      assert((priority >= 0) && (priority <= 10));
      fds_uint32_t weight = 0;

      weight = (11-priority);
      return weight;
      
    }

    fds_uint32_t getNextQueueInPriorityWFQList(fds_uint32_t queue_id) {
      if (queue_desc_map.empty()) {
	return 0;
      }
	auto it = queue_desc_map.find(queue_id);
	if (it != queue_desc_map.end())
	  it++;
	if (it == queue_desc_map.end()) {
	  it = queue_desc_map.begin();
	}
	fds_uint32_t next_queue = it->first;
	return next_queue;
    }

    fds_uint32_t get_non_empty_queue_with_highest_credits();
    void ioProcessForEnqueue(fds_uint32_t queue_id, FDS_IOType *io);
    void ioProcessForDispatch(fds_uint32_t queue_id, FDS_IOType *io);
    fds_uint32_t getNextQueueForDispatch();
    void inc_num_ios_dispatched(unsigned int io_dispatch_type);
    Error assignSpotsToQueue(WFQQueueDesc *qd);
    Error revokeSpotsFromQueue(WFQQueueDesc *qd);

  public:

    QoSWFQDispatcher(FDS_QoSControl *ctrlr, fds_uint64_t total_server_rate,
		     fds_uint32_t maximum_outstanding_ios, fds_log *parent_log);
    Error registerQueue(fds_uint32_t queue_id, FDS_VolumeQueue *queue);
    Error deregisterQueue(fds_uint32_t queue_id);
    Error modifyQueueQosParams(fds_uint32_t queue_id,
			       fds_uint64_t iops_min,
			       fds_uint64_t iops_max,
			       fds_uint32_t prio);

  };
}
