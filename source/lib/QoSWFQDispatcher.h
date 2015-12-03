#include "fds_qos.h"
#include <map>
#include <atomic>

#include "EclipseWorkarounds.h"

namespace fds {

  const unsigned int io_dispatch_type_rate = 0;
  const unsigned int io_dispatch_type_credit = 1;
  const unsigned int io_dispatch_type_priority = 2;

  class WFQQueueDesc {

  public:

    fds_qid_t queue_id;
    fds_uint64_t queue_rate;
    fds_uint32_t queue_priority;
    fds_int64_t rate_based_weight;
    fds_uint32_t priority_based_weight;

    std::vector<fds_uint64_t> rate_based_rr_spots;
    fds_uint32_t num_priority_based_ios_dispatched; // number of ios dispatched in the current round of priority based WFQ;
    fds_uint32_t num_rate_based_credits;
    fds_uint32_t max_rate_based_credits;

    FDS_VolumeQueue *queue;
    alignas(64) std::atomic<unsigned int> num_pending_ios;
    alignas(64) std::atomic<unsigned int> num_outstanding_ios;

    WFQQueueDesc(fds_qid_t q_id,
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

    /**
     * Returns number of pending requests in the queue if queue is
     * in active state or quiescing state; otherwise returns 0
     */
    inline fds_uint32_t pendingActiveCount() const {
        if ((queue->volQState == FDS_VOL_Q_ACTIVE) ||
            (queue->volQState == FDS_VOL_Q_QUIESCING)) {
            return num_pending_ios.load(std::memory_order_relaxed);
        }
        return 0;
    }
  };

  class QoSWFQDispatcher : public FDS_QoSDispatcher {

  private:

    fds_uint64_t total_capacity;
    fds_uint64_t cur_total_min_rate;  // sum of min rates of all queues
    int num_queues;
    std::map<fds_qid_t, WFQQueueDesc *> queue_desc_map;
    std::vector<fds_qid_t> rate_based_qlist;
    fds_uint64_t next_rate_based_spot;
    fds_uint64_t total_rate_based_spots;
    fds_qid_t next_priority_based_queue;

    fds_uint64_t num_ios_dispatched;
    fds_uint64_t num_rate_based_slots_serviced;
    fds_uint64_t last_reset_time;

    fds_uint32_t priority_to_wfq_weight(fds_uint32_t priority) {
      assert((priority >= 0) && (priority <= 10));
      fds_uint32_t weight = 0;

      weight = (11-priority);
      return weight;

    }

    unsigned getNextQueueCount() {
        return queue_desc_map.size();
    }
    fds_qid_t getNextQueueInPriorityWFQList(fds_qid_t queue_id) {
      if (queue_desc_map.empty()) {
	return 0;
      }
	auto it = queue_desc_map.find(queue_id);
	if (it != queue_desc_map.end())
	  ++it;
	if (it == queue_desc_map.end()) {
	  it = queue_desc_map.begin();
	}
	fds_qid_t next_queue = it->first;
	return next_queue;
    }

    fds_qid_t get_non_empty_queue_with_highest_credits();
    void ioProcessForEnqueue(fds_qid_t queue_id, FDS_IOType *io);
    void ioProcessForDispatch(fds_qid_t queue_id, FDS_IOType *io);
    fds_qid_t getNextQueueForDispatch();
    void inc_num_ios_dispatched(unsigned int io_dispatch_type);
    Error assignSpotsToQueue(WFQQueueDesc *qd);
    Error revokeSpotsFromQueue(WFQQueueDesc *qd);

  public:

    QoSWFQDispatcher(FDS_QoSControl *ctrlr, fds_int64_t total_server_iops,
		     fds_uint32_t maximum_outstanding_ios, fds_log *parent_log);
    ~QoSWFQDispatcher() {}
    Error registerQueue(fds_qid_t queue_id, FDS_VolumeQueue *queue);
    Error deregisterQueue(fds_qid_t queue_id);

    using FDS_QoSDispatcher::modifyQueueQosParams;
    Error modifyQueueQosParams(fds_qid_t queue_id,
			       fds_uint64_t iops_min,
			       fds_uint64_t iops_max,
			       fds_uint32_t prio);

  };
}
