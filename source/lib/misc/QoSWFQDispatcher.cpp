/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include "QoSWFQDispatcher.h"

namespace fds {

// Caller needs to hold the qda read lock
void
QoSWFQDispatcher::ioProcessForEnqueue(fds_qid_t queue_id, FDS_IOType *io)
{
    FDS_QoSDispatcher::ioProcessForEnqueue(queue_id, io);
    WFQQueueDesc *qd = queue_desc_map[queue_id];
    auto n_pios = atomic_fetch_add(&(qd->num_pending_ios), (unsigned int)1);
    assert(n_pios >= 0);
}

// Caller needs to hold the qda read lock
void
QoSWFQDispatcher::ioProcessForDispatch(fds_qid_t queue_id, FDS_IOType *io)
{
    FDS_QoSDispatcher::ioProcessForDispatch(queue_id, io);
    WFQQueueDesc *qd = queue_desc_map[queue_id];
    fds_uint32_t n_pios;
    n_pios = atomic_fetch_sub(&(qd->num_pending_ios), (unsigned int)1);
    assert(n_pios >= 1);
}

void
QoSWFQDispatcher::inc_num_ios_dispatched(unsigned int io_dispatch_type)
{
    ++num_ios_dispatched;
    if (num_ios_dispatched >= total_capacity) {
        num_ios_dispatched = 0;
        num_rate_based_slots_serviced = 0;
        last_reset_time = util::getTimeStampMicros();
    }
}

fds_qid_t
QoSWFQDispatcher::get_non_empty_queue_with_highest_credits()
{
    fds_qid_t next_queue = 0;
    fds_qid_t queue_with_highest_credits = 0;
    WFQQueueDesc *next_qd = NULL;
    float highest_credits = 0;
    // std::string credits_str = "";
    for (auto it = queue_desc_map.begin(); it != queue_desc_map.end(); ++it) {
        next_queue = it->first;
        next_qd = it->second;
        // pending IOs if queue is active, otherwise 0
        fds_uint32_t n_pios = next_qd->pendingActiveCount();
        fds_uint32_t n_credits = next_qd->num_rate_based_credits;
        float accumulated_credits = (static_cast<float>(n_credits)) /
                                    next_qd->max_rate_based_credits;
        // credits_str = credits_str + "(" + std::to_string(next_queue) +
        //                "," + std::to_string(n_credits) + "," +
        //                std::to_string(accumulated_credits) + "," +
        //                std::to_string(n_pios) + "), ";
        if ((n_pios > 0) && (accumulated_credits > highest_credits)) {
            queue_with_highest_credits = next_queue;
            highest_credits = accumulated_credits;
        }
    }
    // LOGDEBUG << "Dispatcher: credit state: [" << credits_str << "]";
    return queue_with_highest_credits;
}

// Requires the caller to hold the qda read lock while calling this.
fds_qid_t
QoSWFQDispatcher::getNextQueueForDispatch()
{
    fds_uint64_t current_time = util::getTimeStampMicros();
    fds_uint64_t elapsed_usecs = current_time - last_reset_time;                                    // O(1)
    // ios per second that we have been able to achieve since last reset time.
    // float current_rate = ((float)num_ios_dispatched * 1000000)/ elapsed_usecs;
    float current_guaranteed_ios_rate =
        (static_cast<float>(num_rate_based_slots_serviced) * 1000000)/elapsed_usecs;                // O(1)
    float expected_guaranteed_ios_rate = total_rate_based_spots;                                    // O(1)

    bool running_late = (current_guaranteed_ios_rate < 0.9 * expected_guaranteed_ios_rate);         // O(1)
    bool running_ahead = (current_guaranteed_ios_rate > 1.2 * expected_guaranteed_ios_rate);        // O(1)

    // If we are running ahead, let's treat this as an open slot.
    // If we are running late, let's skip over open slots and serve only guaranteed slots

    fds_qid_t next_queue = 0;                                                                       // O(1)

    if (!running_ahead) {                                                                           // O(1)
        // Step 1: Let's first look at the current rate_based slot to see if
        // there is a valid non-empty queue there. If we are running late,
        // we will simply move to the next rate based spot if the current
        // rate based spot is empty or does not have IOs queued up.
        // If we are not running late, we will treat the empty/unused slot
        // as an open slot and move to the priority based WFQ list immediately.
        fds_uint32_t n_pios = 0;                                                                    // O(1)
        WFQQueueDesc *next_qd = NULL;                                                               // O(1)
        do {
            next_queue = rate_based_qlist[next_rate_based_spot];                                    //     O(1)
            next_rate_based_spot = (next_rate_based_spot + 1) % total_capacity;                     //     O(1)
            if (next_queue != 0) {                                                                  //     O(1)
                ++num_rate_based_slots_serviced;                                                    //     O(1)
                current_guaranteed_ios_rate =
                    (static_cast<float>(num_rate_based_slots_serviced * 1000000))/elapsed_usecs;    //     O(1)

                next_qd = queue_desc_map[next_queue];                                               //     O(1) avg, O(size()) worst
                // pending IOs if queue is active, otherwise 0
                n_pios = next_qd->pendingActiveCount();

                if ((n_pios == 0) &&
                    (next_qd->num_rate_based_credits < next_qd->max_rate_based_credits)) {          //     O(1)
                    next_qd->num_rate_based_credits++;                                              //     O(1)
                    LOGDEBUG << "Dispatcher: Incrementing credit for queue " << next_queue
                             << " to " << next_qd->num_rate_based_credits;
                }

                running_late = (current_guaranteed_ios_rate < (0.9 * expected_guaranteed_ios_rate));//     O(1)
            }
        } while ((running_late) && ((next_queue == 0) || (n_pios == 0)));

        if ((next_queue != 0) && (n_pios > 0)) {                                                    // O(1)
            inc_num_ios_dispatched(io_dispatch_type_rate);
            LOGDEBUG << "Dispatcher: picking next rate based queue " << next_queue
                     << " for slot " << next_rate_based_spot-1
                     << "; current throttle state - ("
                     << current_guaranteed_ios_rate << ":" << expected_guaranteed_ios_rate  << ", "
                     << running_ahead << ", " << running_late << ")";
            return next_queue;
        }
        // Now next_queue is either null or n_pios for that queue is 0
        // and we are moving over to priority based dispatch.
        // This can only happen if we are not running late.
        assert(running_late == false);
    }


    // Step 2:: This is an open spot.
    // Let's first check if there is any nonempty queue with accumulated rate based credit.
    // If there are such queues, the queue with the highest accumulated credit gets the priority.

    next_queue = get_non_empty_queue_with_highest_credits();
    if (next_queue != 0) {                                                                          // O(1)
        WFQQueueDesc *next_qd = queue_desc_map[next_queue];                                         // O(1) avg, O(size()) worst
        assert(next_qd->num_rate_based_credits > 0);
        // assert(next_qd->num_rate_based_credits <= next_qd->max_rate_based_credits);
        next_qd->num_rate_based_credits--;                                                          // O(1)
        inc_num_ios_dispatched(io_dispatch_type_credit);
        LOGDEBUG << "Dispatcher: picking next credit based queue " << next_queue
                 << "(" << next_qd->num_rate_based_credits << ") for slot "
                 << next_rate_based_spot-1
                 << "; current throttle state - ("
                 << current_guaranteed_ios_rate << ":"
                 << expected_guaranteed_ios_rate  << ", "
                 << running_ahead << ", " << running_late << ")";
        return next_queue;
    }

    // Step 3: Rate based spot was empty and there was no queue with accumulated credit.
    // Let's now pick the next q in the priority based WFQ list
    next_queue = next_priority_based_queue;                                                         // O(1)
    WFQQueueDesc *next_qd = (queue_desc_map.count(next_queue) == 0) ? NULL :
                                                                      queue_desc_map[next_queue];   // O(1) avg, O(size()) worst
    fds_uint32_t n_pios = 0;                                                                        // O(1)
    if (next_qd) {                                                                                  // O(1)
        n_pios = next_qd->pendingActiveCount();  // pending IOs if queue is active, otherwise 0
    }

    unsigned totalQueueCount = getNextQueueCount();
    unsigned queueCounted = 0;
    while (((!next_qd) || (n_pios == 0)) && (totalQueueCount && (queueCounted < totalQueueCount))) { // O(1) condition / O(?) loop
        if (next_qd) {                                                                              //     O(1)
            next_qd->num_priority_based_ios_dispatched = 0;                                         //     O(1)
        }

        next_queue = next_priority_based_queue = getNextQueueInPriorityWFQList(next_queue);
        next_qd = (queue_desc_map.count(next_queue) == 0)? NULL:queue_desc_map[next_queue];         //     O(1) avg, O(size()) worst
        if (next_qd) {                                                                              //     O(1)
            next_qd->num_priority_based_ios_dispatched = 0;                                         //     O(1)
            n_pios = next_qd->pendingActiveCount();  // pending IOs if queue is active, otherwise 0
        }
        ++queueCounted;
    }

    if ((next_qd == NULL) || (n_pios == 0)) {
        // none of the active queues have any IOs pending, will not dispatch
        return 0;
    }

    assert(queue_desc_map.count(next_queue) != 0);
    assert(next_qd != NULL);
    assert(n_pios > 0);
    // Step3 end: next_queue is the queue we are going to dispatch from. next_qd is it's descriptor
    LOGTRACE << "Dispatcher: picking next priority based queue " << next_queue
             << " for slot " << next_rate_based_spot-1
             << "; current throttle state - ("
             << current_guaranteed_ios_rate << ":" << expected_guaranteed_ios_rate  << ", "
             << running_ahead << ", " << running_late << ")";


    // Step 3: Now we will update the priority based WFQ state for the next iteration

    next_qd->num_priority_based_ios_dispatched++;

    if (next_qd->num_priority_based_ios_dispatched >= next_qd->priority_based_weight) {
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


QoSWFQDispatcher::QoSWFQDispatcher(FDS_QoSControl *ctrlr,
                                   fds_int64_t total_server_iops,
                                   fds_uint32_t maximum_outstanding_ios,
                                   bool bypass_disp,
                                   fds_log *parent_log)
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
    bypass_dispatcher = bypass_disp;
    LOGNOTIFY << "Will bypass QoS? " << bypass_dispatcher;

    num_ios_dispatched = 0;
    num_rate_based_slots_serviced = 0;
    last_reset_time = util::getTimeStampMicros();

    cur_total_min_rate = 0;
    total_capacity = total_svc_iops = total_server_iops;
    rate_based_qlist.clear();
    next_rate_based_spot = 0;
    total_rate_based_spots = 0;
    for (uint i = 0; i < total_capacity; ++i) {
        // "0" is a special queue_id indicating this spot is assigned
        // to no queue. Open to be used for priority based WFQ assignment
        rate_based_qlist.push_back(0);
    }
    num_queues = 0;
    queue_desc_map.clear();
    next_priority_based_queue = 0;
}

Error
QoSWFQDispatcher::assignSpotsToQueue(WFQQueueDesc *qd)
{
    Error err(ERR_OK);

    FDS_VolumeQueue *queue = qd->queue;

    fds_uint64_t current_spot = 0;
    // std::string spot_list_string = "(";
    for (fds_int64_t i = 0; i < queue->iops_assured; ++i) {
        fds_uint64_t num_spots_searched = 0;
        while ((rate_based_qlist[current_spot] != 0) &&
                (num_spots_searched < total_capacity)) {
            current_spot = (current_spot + 1) % total_capacity;
            ++num_spots_searched;
        }
        assert(rate_based_qlist[current_spot] == 0);
        rate_based_qlist[current_spot] = qd->queue_id;
        // spot_list_string = spot_list_string + std::to_string(current_spot) + ", ";
        qd->rate_based_rr_spots.push_back(current_spot);
        current_spot =
            (current_spot + total_capacity / queue->iops_assured) % total_capacity;
    }
    // spot_list_string = spot_list_string + ")";
    total_rate_based_spots += queue->iops_assured;
    // LOGDEBUG << "Dispatcher: assigning to queue " << qd->queue_id
    //         << " slots - " << spot_list_string;

    return err;
}

Error
QoSWFQDispatcher::revokeSpotsFromQueue(WFQQueueDesc *qd)
{
    Error err(ERR_OK);

    // FDS_VolumeQueue *queue = qd->queue;

    for (fds_uint64_t i = 0; i < qd->queue_rate; ++i) {
        fds_uint32_t next_rate_based_spot = qd->rate_based_rr_spots[i];
        // This is now a free spot, that could be used for
        // priority-based WFQ assignment
        rate_based_qlist[next_rate_based_spot] = 0;
    }
    qd->rate_based_rr_spots.clear();
    total_rate_based_spots -= qd->queue_rate;

    return err;
}

Error
QoSWFQDispatcher::registerQueue(fds_qid_t queue_id, FDS_VolumeQueue *queue)
{
    Error err(ERR_OK);

    if (queue->iops_assured == 0) {
        queue->iops_assured = 1;
    }

    WFQQueueDesc *qd = new WFQQueueDesc(queue_id, queue, queue->iops_assured, queue->priority);
    qd->rate_based_weight = queue->iops_assured;
    qd->priority_based_weight = priority_to_wfq_weight(queue->priority);
    qd->num_pending_ios = ATOMIC_VAR_INIT(0);
    qd->num_outstanding_ios = ATOMIC_VAR_INIT(0);
    qd->num_priority_based_ios_dispatched = 0;
    qd->num_rate_based_credits = 0;
    // Can accumulate up to half a second worth of spots.
    qd->max_rate_based_credits = queue->iops_assured / 2 + 1;

    qda_lock.write_lock();
    // do not allow registering the queue that will exceed total
    // server rate, so that we can still promise min iops to everyone
    if ((queue->iops_assured + cur_total_min_rate) > total_capacity) {
        qda_lock.write_unlock();
        delete qd;
        LOGERROR << "QoSWFQDispatcher: could not admit this volume, because "
                 << " total min iops would exceed total rate" << std::endl
                 << "  cur_total_min_rate " << cur_total_min_rate
                 << " volume's assured rate " << queue->iops_assured
                 << " total server rate " << total_capacity;
        return Error(ERR_EXCEED_MIN_IOPS);
    }

    err = FDS_QoSDispatcher::registerQueueWithLockHeld(queue_id, queue);
    if (err != ERR_OK) {
        qda_lock.write_unlock();
        return err;
    }

    if (queue_desc_map.count(queue_id) != 0) {
        qda_lock.write_unlock();
        delete qd;
        return Error(ERR_DUPLICATE);
    }

    // we are admitting this queue, update total min rate
    cur_total_min_rate += queue->iops_assured;

    // Now fill the vacant spots in the rate based qlist based on the queue_rate
    // Start at the first vacant spot and fill at intervals of capacity/queue_rate.
    assignSpotsToQueue(qd);
    queue_desc_map[queue_id] = qd;
    queue_map[queue_id] = queue;
    qda_lock.write_unlock();

    return err;
}

Error
QoSWFQDispatcher::modifyQueueQosParams(fds_qid_t queue_id,
                                       fds_uint64_t iops_min,
                                       fds_uint64_t iops_max,
                                       fds_uint32_t prio)
{
    Error err(ERR_OK);
    fds_uint64_t q_min_rate = iops_min;

    if (q_min_rate < 1) {
        q_min_rate = 1;
    }

    qda_lock.write_lock();
    if  (queue_desc_map.count(queue_id) == 0) {
        qda_lock.write_unlock();
        return Error(ERR_NOT_FOUND);
    }
    WFQQueueDesc *qd = queue_desc_map[queue_id];
    fds_verify(cur_total_min_rate >= qd->queue_rate);

    // check if we can admit modified min iops
    if ((q_min_rate > qd->queue_rate) &&
        ((q_min_rate - qd->queue_rate + cur_total_min_rate) > total_capacity)) {
        qda_lock.write_unlock();
        LOGERROR << "QoSWFQDispatcher: could not admit increased vol rate, because "
                 << " total min iops would exceed total rate" << std::endl
                 << "  cur_total_min_rate " << cur_total_min_rate
                 << " volume's old min rate " << qd->queue_rate
                 << " volume's new min rate " << q_min_rate
                 << " total server rate " << total_capacity;
        return Error(ERR_EXCEED_MIN_IOPS);
    }

    err = FDS_QoSDispatcher::modifyQueueQosWithLockHeld(queue_id, iops_min, iops_max, prio);
    if (!err.ok()) {
        qda_lock.write_unlock();
        return err;
    }

    // Save the old parameters
    // fds_uint32_t old_rb_wt = qd->rate_based_weight;
    // fds_uint32_t old_queue_rate = qd->queue_rate;
    // fds_uint32_t old_pb_wt = qd->priority_based_weight;
    // fds_uint32_t old_max_rb_credits = qd->max_rate_based_credits;

    // First remove this queue from all the old spots allocated to it.
    revokeSpotsFromQueue(qd);
    cur_total_min_rate -= qd->queue_rate;  // sub old rate
    cur_total_min_rate += q_min_rate;  // add new rate

    // Update with new parameters
    qd->queue_rate = q_min_rate;
    qd->queue_priority = prio;
    qd->rate_based_weight = q_min_rate;
    qd->priority_based_weight = priority_to_wfq_weight(prio);
    // Can accumulate up to half a second worth of spots.
    qd->max_rate_based_credits = q_min_rate / 2 + 1;

    // Now fill the vacant spots in the rate based qlist based on the new queue_rate
    // Start at the first vacant spot and fill at intervals of capacity/queue_rate.
    assignSpotsToQueue(qd);

    // Note, if this queue is the current next_priority_based_queue and the priority
    // for this queue is being changed, situation will auto-correct itself
    // in the next priority based dispatch. This can happen for example
    // when the new priority based weight is lowered and is now less than
    // the num_priority_based_ios_dispatched for this queue for this round.

    qda_lock.write_unlock();

    return err;
}

Error
QoSWFQDispatcher::deregisterQueue(fds_qid_t queue_id)
{
    Error err(ERR_OK);

    qda_lock.write_lock();
    if  (queue_desc_map.count(queue_id) == 0) {
        qda_lock.write_unlock();
        err = ERR_INVALID_ARG;
        return err;
    }
    WFQQueueDesc *qd = queue_desc_map[queue_id];

    // update current total min rate
    assert(qd->queue_rate  <= cur_total_min_rate);
    cur_total_min_rate -= qd->queue_rate;

    revokeSpotsFromQueue(qd);

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
}  // namespace fds
