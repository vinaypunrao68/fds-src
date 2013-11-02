#include "QoSWFQDispatcher.h"

namespace fds {

  // Caller needs to hold the qda read lock
  void QoSWFQDispatcher::ioProcessForEnqueue(fds_uint32_t queue_id, FDS_IOType *io)
  {
    FDS_QoSDispatcher::ioProcessForEnqueue(queue_id, io);
    WFQQueueDesc *qd = queue_desc_map[queue_id];
    fds_uint32_t n_pios;
    n_pios = atomic_load(&qd->num_pending_ios);
    while (n_pios >= MAX_PENDING_IOS_PER_VOLUME) {
      qda_lock.read_unlock();
      boost::this_thread::sleep(boost::posix_time::microseconds(1000000/qd->queue_rate));
      qda_lock.read_lock();
      n_pios = atomic_load(&qd->num_pending_ios);
    }

    n_pios = atomic_fetch_add(&(qd->num_pending_ios), (unsigned int)1);
    assert(n_pios >= 0);
  }

  // Caller needs to hold the qda read lock
  void QoSWFQDispatcher::ioProcessForDispatch(fds_uint32_t queue_id, FDS_IOType *io)
  {
    FDS_QoSDispatcher::ioProcessForDispatch(queue_id, io);
    WFQQueueDesc *qd = queue_desc_map[queue_id];
    fds_uint32_t n_pios;
    n_pios = atomic_fetch_sub(&(qd->num_pending_ios), (unsigned int)1);
    assert(n_pios >= 1);
  }

  void QoSWFQDispatcher::inc_num_ios_dispatched(unsigned int io_dispatch_type) {
    num_ios_dispatched++;
    if (num_ios_dispatched >= total_capacity) {
      num_ios_dispatched = 0;
      num_rate_based_slots_serviced = 0;
      last_reset_time = boost::posix_time::microsec_clock::universal_time();
    }
  }

  fds_uint32_t QoSWFQDispatcher::get_non_empty_queue_with_highest_credits() {
    fds_uint32_t next_queue = 0;
    fds_uint32_t queue_with_highest_credits = 0;
    WFQQueueDesc *next_qd = NULL;
    float highest_credits = 0;
    std::string credits_str = "";
    for (auto it=queue_desc_map.begin(); it != queue_desc_map.end(); ++it) {
      next_queue = it->first;
      next_qd = it->second;
      fds_uint32_t n_pios = atomic_load(&(next_qd->num_pending_ios));
      fds_uint32_t n_credits = next_qd->num_rate_based_credits;
      float accumulated_credits = ((float) n_credits)/next_qd->max_rate_based_credits;
      credits_str = credits_str + "(" + std::to_string(next_queue) + "," + std::to_string(n_credits)
	+ "," + std::to_string(accumulated_credits) + "," + std::to_string(n_pios) + "), ";
      if ((n_pios > 0) && (accumulated_credits > highest_credits)) {
	queue_with_highest_credits = next_queue;
	highest_credits = accumulated_credits;
      }
    }
    FDS_PLOG(qda_log) << "Dispatcher: credit state: [" << credits_str << "]";
    return queue_with_highest_credits;
  }


  fds_uint32_t QoSWFQDispatcher::getNextQueueForDispatch() {
      
    boost::posix_time::ptime current_time = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration elapsed_time = current_time - last_reset_time;
    fds_uint64_t elapsed_usecs = elapsed_time.total_microseconds();
    float current_rate = ((float)num_ios_dispatched * 1000000)/ elapsed_usecs; // ios per second that we have been able to achieve since last reset time.
    float current_guaranteed_ios_rate = ((float)num_rate_based_slots_serviced * 1000000)/elapsed_usecs;
    float expected_guaranteed_ios_rate = total_rate_based_spots;

    bool running_late = (current_guaranteed_ios_rate < 0.9 * expected_guaranteed_ios_rate);
    bool running_ahead = (current_guaranteed_ios_rate > 1.2 * expected_guaranteed_ios_rate); 

    // If we are running ahead, let's treat this as an open slot. If we are running late, let's skip over open slots and serve only guaranteed slots

    fds_uint32_t next_queue = 0; 
      

    if (!running_ahead) {
      // Step 1: Let's first look at the current rate_based slot to see if there is a valid non-empty queue there
      // If we are running late, we will simply move to the next rate based spot if the current rate based spot
      // is empty or does not have IOs queued up.
      // If we are not running late, we will treat the empty/unused slot as an open slot and move to the priority based WFQ list immediately.
      fds_uint32_t n_pios = 0;
      WFQQueueDesc *next_qd = NULL;
      do {
	next_queue = rate_based_qlist[next_rate_based_spot];
	next_rate_based_spot = (next_rate_based_spot + 1) % total_capacity;
	if (next_queue != 0) {

	  num_rate_based_slots_serviced++;
	  current_guaranteed_ios_rate = ((float)num_rate_based_slots_serviced * 1000000)/elapsed_usecs;

	  next_qd = queue_desc_map[next_queue];
	  n_pios = atomic_load(&(next_qd->num_pending_ios));
	  if ((n_pios == 0) && (next_qd->num_rate_based_credits < next_qd->max_rate_based_credits)) { 
	    next_qd->num_rate_based_credits++;
	    FDS_PLOG(qda_log) << "Dispatcher: Incrementing credit for queue " << next_queue
			      << " to " << next_qd->num_rate_based_credits;
	  }

	  running_late = (current_guaranteed_ios_rate < 0.9 * expected_guaranteed_ios_rate);
	}	
      } while ((running_late) && ((next_queue == 0) || (n_pios == 0)));

      if ((next_queue != 0) && (n_pios > 0)) {
	inc_num_ios_dispatched(io_dispatch_type_rate);
	FDS_PLOG(qda_log) << "Dispatcher: picking next rate based queue " << next_queue
			  << " for slot " << next_rate_based_spot-1
	                  << "; current throttle state - ("
			  << current_guaranteed_ios_rate << ":" << expected_guaranteed_ios_rate  << ", " 
	                  << running_ahead << ", " << running_late << ")";
	return next_queue;
      }
      // Now next_queue is either null or n_pios for that queue is 0 
      // and we are moving over to priority based dispatch.
      // This can only happen if we are not running late.
      assert (running_late == false);
    }

    
    // Step 2:: This is an open spot.
    // Let's first check if there is any nonempty queue with accumulated rate based credit.
    // If there are such queues, the queue with the highest accumulated credit gets the priority.

    next_queue = get_non_empty_queue_with_highest_credits();
    if (next_queue != 0) {
      WFQQueueDesc *next_qd = queue_desc_map[next_queue];
      assert(next_qd->num_rate_based_credits > 0);
      assert(next_qd->num_rate_based_credits <= next_qd->max_rate_based_credits);
      next_qd->num_rate_based_credits--;
      inc_num_ios_dispatched(io_dispatch_type_credit);
      FDS_PLOG(qda_log) << "Dispatcher: picking next credit based queue " << next_queue
			<< "(" << next_qd->num_rate_based_credits << ") for slot " << next_rate_based_spot-1
			<< "; current throttle state - ("  
			<< current_guaranteed_ios_rate << ":" << expected_guaranteed_ios_rate  << ", " 
			<< running_ahead << ", " << running_late << ")";
      return next_queue;
    }


      // Step 3: Rate based spot was empty and there was no queue with accumulated credit. 
      // Let's now pick the next q in the priority based WFQ list
      next_queue = next_priority_based_queue;
      WFQQueueDesc *next_qd = (queue_desc_map.count(next_queue) == 0)? NULL:queue_desc_map[next_queue];
      fds_uint32_t n_pios = 0;
      if (next_qd) {
	n_pios = atomic_load(&(next_qd->num_pending_ios));
      }

      while ((!next_qd) || (n_pios == 0)){
	if (next_qd)
	  next_qd->num_priority_based_ios_dispatched = 0;
	
	next_queue = next_priority_based_queue = getNextQueueInPriorityWFQList(next_queue);
	next_qd = (queue_desc_map.count(next_queue) == 0)? NULL:queue_desc_map[next_queue];
	if (next_qd) {
	  next_qd->num_priority_based_ios_dispatched = 0;
	  n_pios = atomic_load(& next_qd->num_pending_ios);
	}
      }

      assert(queue_desc_map.count(next_queue) != 0);
      assert(next_qd != NULL);
      assert(n_pios > 0);
      // Step3 end: next_queue is the queue we are going to dispatch from. next_qd is it's descriptor
      FDS_PLOG(qda_log) << "Dispatcher: picking next priority based queue " << next_queue
	                << " for slot " << next_rate_based_spot-1
			<< "; current throttle state - (" 
			<< current_guaranteed_ios_rate << ":" << expected_guaranteed_ios_rate  << ", " 
			<< running_ahead << ", " << running_late << ")";


      // Step 3: Now we will update the priority based WFQ state for the next iteration

      next_qd->num_priority_based_ios_dispatched ++;

      if (next_qd->num_priority_based_ios_dispatched == next_qd->priority_based_weight) {
	next_qd->num_priority_based_ios_dispatched = 0;
	next_priority_based_queue = getNextQueueInPriorityWFQList(next_queue);
	if (queue_desc_map.count(next_priority_based_queue) > 0) {
	  next_qd = queue_desc_map[next_priority_based_queue];
	  next_qd->num_priority_based_ios_dispatched = 0;
	}
      }

      inc_num_ios_dispatched(io_dispatch_type_priority);     
      return next_queue;

  }


  QoSWFQDispatcher::QoSWFQDispatcher(FDS_QoSControl *ctrlr, fds_uint64_t total_server_rate,
				     fds_uint32_t maximum_outstanding_ios, fds_log *parent_log)
  {
      parent_ctrlr = ctrlr;
      if (parent_log == NULL) {
	qda_log = new fds_log("qda", "logs");
      } else {
	qda_log = parent_log;
      }
      max_outstanding_ios = maximum_outstanding_ios;
      num_pending_ios = ATOMIC_VAR_INIT(0);
      num_outstanding_ios = ATOMIC_VAR_INIT(0);

      stats_mutex = new fds_mutex("QoSDispatcherMutex");

      num_ios_dispatched = 0;
      num_rate_based_slots_serviced = 0;
      last_reset_time = boost::posix_time::microsec_clock::universal_time();

      total_capacity = total_svc_rate = total_server_rate;
      rate_based_qlist.clear();
      next_rate_based_spot = 0;
      total_rate_based_spots = 0;
      for (int i = 0; i < total_capacity; i++) {
	rate_based_qlist.push_back(0); // "0" is a special queue_id indicating this spot is assigned to no queue. Open to be used for priority based WFQ assignment
      }
      num_queues = 0;
      queue_desc_map.clear();
      next_priority_based_queue = 0;
  }


  Error QoSWFQDispatcher::registerQueue(fds_uint32_t queue_id, FDS_VolumeQueue *queue) {

      Error err(ERR_OK);

      assert(queue_id > 0);

      if (queue->iops_min == 0) {
	queue->iops_min = 1;
      }

      WFQQueueDesc *qd = new WFQQueueDesc(queue_id, queue, queue->iops_min, queue->priority);
      qd->rate_based_weight = queue->iops_min;
      qd->priority_based_weight = priority_to_wfq_weight(queue->priority);
      qd->num_pending_ios = ATOMIC_VAR_INIT(0);
      qd->num_outstanding_ios = ATOMIC_VAR_INIT(0);
      qd->num_priority_based_ios_dispatched = 0;
      qd->num_rate_based_credits = 0;
      qd->max_rate_based_credits = queue->iops_min/2 + 1; // Can accumulate up to half a second worth of spots. 

      qda_lock.write_lock();
      err = FDS_QoSDispatcher::registerQueueWithLockHeld(queue_id, queue);
      if (err != ERR_OK) {
	qda_lock.write_unlock();
	return err;
      }

      if (queue_desc_map.count(queue_id) != 0) {
	qda_lock.write_unlock();
	delete qd;
	err = ERR_DUPLICATE;
	return err;
      }
 

      // Now fill the vacant spots in the rate based qlist based on the queue_rate
      // Start at the first vacant spot and fill at intervals of capacity/queue_rate.
      fds_uint64_t current_spot = 0;
      std::string spot_list_string = "(";
      for (fds_uint64_t i = 0; i < queue->iops_min; i++) {
	fds_uint64_t num_spots_searched = 0;
	while ((rate_based_qlist[current_spot] != 0) && (num_spots_searched < total_capacity)) {
	  current_spot = (current_spot+1) % total_capacity;
	  num_spots_searched ++;
	}
	assert(rate_based_qlist[current_spot] == 0);
	rate_based_qlist[current_spot] = queue_id;
	spot_list_string = spot_list_string + std::to_string(current_spot) + ", "; 
	qd->rate_based_rr_spots.push_back(current_spot);
	current_spot = (current_spot + (int) total_capacity/queue->iops_min) % total_capacity;
      }
      spot_list_string = spot_list_string + ")";
      total_rate_based_spots += queue->iops_min;
      FDS_PLOG(qda_log) << "Dispatcher: assigning to queue " << queue_id
			<< " slots - " << spot_list_string;
      queue_desc_map[queue_id] = qd;
      queue_map[queue_id] = queue;
      qda_lock.write_unlock();

      return err;

  }

  Error QoSWFQDispatcher::deregisterQueue(fds_uint32_t queue_id) {

      Error err(ERR_OK);

      qda_lock.write_lock();
      if  (queue_desc_map.count(queue_id) == 0) {
	qda_lock.write_unlock();
	err = ERR_INVALID_ARG;
	return err;
      }
      WFQQueueDesc *qd = queue_desc_map[queue_id];
      for (fds_uint64_t i = 0; i < qd->queue_rate; i++) {
	fds_uint32_t next_rate_based_spot = qd->rate_based_rr_spots[i];
	rate_based_qlist[next_rate_based_spot] = 0; // This is now a free spot, that could be used for priority-based WFQ assignment
      }
      total_rate_based_spots -= qd->queue_rate;
      if (next_priority_based_queue == queue_id) {
	next_priority_based_queue = getNextQueueInPriorityWFQList(queue_id);
	WFQQueueDesc *next_qd = queue_desc_map[next_priority_based_queue];
	next_qd->num_priority_based_ios_dispatched = 0;
      }

      queue_desc_map.erase(queue_id);
      queue_map.erase(queue_id);
      err = FDS_QoSDispatcher::deregisterQueueWithLockHeld(queue_id);
      qda_lock.write_unlock();
      delete qd;

      return err;

  }
}
