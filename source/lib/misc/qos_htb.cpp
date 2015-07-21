/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 *  Hierarchical token bucket algorithm
 *
 */
#include "qos_htb.h"

namespace fds {

/***** QoSHTBDispatcher implementation ******/
QoSHTBDispatcher::QoSHTBDispatcher(FDS_QoSControl *ctrl,
                                   fds_log *log,
                                   fds_int64_t _total_iops)
    : FDS_QoSDispatcher(ctrl, log, _total_iops),
    total_iops(_total_iops),
    total_assured_rate(0),
    max_burst_size(20),
    wait_time_microsec(DEFAULT_ASSURED_WAIT_MICROSEC),
    avail_pool(_total_iops, max_burst_size),
    current_throttle_level(11)
{
    /* TODO: we need a good per-volume burst size value, sholdn't this be a max queue size at SM/DM? */
    default_que_burst_size = 20;

    last_dispatch_qid = 0; /* this should be an invalid id */
}

/* Returns ERR_INVALID_ARG if _total_rate < sum of all volumes' minimum rates */
Error
QoSHTBDispatcher::modifyTotalRate(fds_int64_t _total_iops)
{
    /* do not modify if _total_rate is below total assured_rate of all volumes */
    if (_total_iops <= total_assured_rate) {
        /* for now we are not letting total rate go below volumes' total min rate */
        return ERR_INVALID_ARG;
    }
    total_iops = _total_iops;
    /* only need to modify avail rate */
    avail_pool.modifyRate(total_iops - total_assured_rate);
    return ERR_OK;
}

Error
QoSHTBDispatcher::registerQueue(fds_qid_t queue_id,
                                FDS_VolumeQueue *queue)
{
    fds_int64_t new_total_min_rate = 0;
    fds_int64_t new_total_avail_rate = 0;
    fds_int64_t q_assured_rate = queue->iops_assured;
    fds_int64_t q_throttle_rate = queue->iops_throttle;

    /* If iops_throttle == 0, means no throttle, set throttle_rate to a very high value */
    if (q_throttle_rate == 0) {
        q_throttle_rate = HTB_QUEUE_RATE_INFINITE_MAX;
    }

    /* since highest priority queues will get all the slack, give some minimum
     * iops to queues iops_assured == 0 */
    if (q_assured_rate < HTB_QUEUE_RATE_MIN) {
        q_assured_rate = HTB_QUEUE_RATE_MIN;
    }

    /* we need a new state to control new queue */
    auto qstate = queue_state_type(new TBQueueState(queue_id,
                                                    q_assured_rate,
                                                    q_throttle_rate,
                                                    queue->priority,
                                                    wait_time_microsec,
                                                    default_que_burst_size));
    if (!qstate) {
        LOGERROR << "QoSHTBDispatcher: failed to allocate memory for queue state for queue: " << queue_id;
        return ERR_MAX;
    }

    /* for now, do not allow total assured rate to increase above total rate -- return error
     *  TODO: should we scale down all volumes' assured rates? */
    qda_lock.write_lock();

    // we also do not want total assured rate to be equal total rate, so that we have at least
    // one IOP for available (non-reserved rate)
    if ((q_assured_rate + total_assured_rate) >= total_iops) {
        qda_lock.write_unlock();
        LOGERROR << "QoSHTBDispatcher: invalid qos rates.  q_assured_rate: " << q_assured_rate
                 << " total_assured_rate: " << total_assured_rate << " total_iops: " << total_iops;
        return ERR_EXCEED_MIN_IOPS;
    }

    /* call base class to actually add queue */
    auto err = FDS_QoSDispatcher::registerQueueWithLockHeld(queue_id, queue);
    if (!err.ok()) {
        qda_lock.write_unlock();
        return err;
    }

    /* base class already checked that queue_id is valid and not already registered */
    /* update total assured rate and avail rate */
    total_assured_rate += q_assured_rate;
    new_total_min_rate = total_assured_rate;
    new_total_avail_rate = avail_pool.getRate();
    if (new_total_avail_rate > q_assured_rate) {
        new_total_avail_rate -= q_assured_rate;
    } else {
        /* for now we allow total_min_rate to exceed total_rate, means avail rate is 0,
         * Even if avail_pool does not generate any tokens, it still gets unused
         * assured tokens (from min_iops reservation) and shares them with other queues */
        new_total_avail_rate = 0;
    }
    avail_pool.modifyRate(new_total_avail_rate);

    /* add queue state to map */
    qstate_map[queue_id].swap(qstate);
    qda_lock.write_unlock();

    LOGNOTIFY << "QosHTBDispatcher: registered queue 0x" << std::hex << queue_id << std::dec
              << "with iops_assured=" << queue->iops_assured
              << "; iops_throttle=" << queue->iops_throttle
              << "; prio=" << queue->priority
              << "; total_min_rate " << new_total_min_rate
              << ", total_avail_rate " << new_total_avail_rate
              << " result: " << err.GetErrstr();

    return err;
}

Error
QoSHTBDispatcher::modifyQueueQosParams(fds_qid_t queue_id,
                                       fds_int64_t iops_assured,
                                       fds_int64_t iops_throttle,
                                       fds_uint32_t prio)
{
    fds_uint64_t new_total_min_rate = 0;
    fds_int64_t new_total_avail_iops = 0;
    fds_int64_t q_assured_rate = iops_assured;
    fds_int64_t q_throttle_rate = iops_throttle;

    /* If iops_throttle == 0, means no max, set throttle_rate to a very high value */
    if (q_throttle_rate == 0) {
        q_throttle_rate = HTB_QUEUE_RATE_INFINITE_MAX;
    }

    /* since highest priority queues will get all the slack, give some minimum
     * iops to queues iops_assured == 0 */
    if (q_assured_rate < HTB_QUEUE_RATE_MIN) {
        q_assured_rate = HTB_QUEUE_RATE_MIN;
    }

    qda_lock.write_lock();

    auto qstate_it = qstate_map.find(queue_id);
    if (qstate_map.end() == qstate_it) {
      qda_lock.write_unlock();
      return ERR_NOT_FOUND;
    }
    auto& qstate = qstate_it->second;
    fds_assert(qstate);

    /* for now, do not allow total assured rate to increase above total rate -- return error
     *  TODO: should we scale down all volumes' assured rates? */
    // we also do not want total assured rate to be equal total rate, so that we have at least
    // one IOP for available (non-reserved rate)
    if ((q_assured_rate > qstate->assured_rate) &&
        ((q_assured_rate - qstate->assured_rate + total_assured_rate) >= total_iops)) {
        qda_lock.write_unlock();
        LOGERROR << "QoSHTBDispatcher: invalid qos rates.  q_assured_rate: "
                 << q_assured_rate << " total_throttle_rate: "
                 << total_assured_rate - qstate->assured_rate + q_assured_rate
                 << "  total_iops: " << total_iops;
        return ERR_EXCEED_MIN_IOPS;
    }

    /* call base class to actually modify queue qos params */
    auto err = FDS_QoSDispatcher::modifyQueueQosWithLockHeld(queue_id,
                                                        q_assured_rate,
                                                        q_throttle_rate,
                                                        prio);
    if (!err.ok()) {
        qda_lock.write_unlock();
        return err;
    }

    /* update total assured rate and avail rate */
    fds_verify(total_assured_rate >= qstate->assured_rate);
    total_assured_rate = total_assured_rate - qstate->assured_rate + q_assured_rate;
    new_total_min_rate = total_assured_rate;
    new_total_avail_iops = avail_pool.getRate() + qstate->assured_rate;
    if (new_total_avail_iops > q_assured_rate) {
        new_total_avail_iops -= q_assured_rate;
    } else {
        /* for now we allow total_min_rate to exceed total_rate, means avail rate is 0,
         * Even if avail_pool does not generate any tokens, it still gets unused
         * assured tokens (from min_iops reservation) and shares them with other queues */
        new_total_avail_iops = 0;
    }
    avail_pool.modifyRate(new_total_avail_iops);

    /* modify queue state params */
    qstate->assured_rate = q_assured_rate;
    qstate->throttle_rate = q_throttle_rate;
    qstate->priority = prio;
    /* since we may also have a throttle level set, this func will change effective min/max rates
     * of token bucket based on iops min and max but also a current throttle level */
    setQueueThrottleLevel(qstate, current_throttle_level);
    qda_lock.write_unlock();

    LOGNOTIFY << "QosHTBDispatcher: modified queue " << queue_id
              << "new q_assured_rate=" << q_assured_rate
              << "; q_throttle_rate=" << q_throttle_rate
              << "; prio=" << prio
              << "; total_min_rate " << new_total_min_rate
              << ", total_avail_iops " << new_total_avail_iops;

    return err;
}

Error
QoSHTBDispatcher::deregisterQueue(fds_qid_t queue_id)
{
    fds_uint64_t new_total_min_rate = 0;
    fds_uint64_t new_total_avail_rate = 0;

    qda_lock.write_lock();
    /* call base class to remove queue */
    auto err = FDS_QoSDispatcher::deregisterQueueWithLockHeld(queue_id);
    /* if error, still try to remove queue state first before returning */

    auto qstate_it = qstate_map.find(queue_id);
    if (qstate_map.end() == qstate_it) {
        qda_lock.write_unlock();
        return ERR_NOT_FOUND; /* we probably got same error from base class, but good to check if queue state exists */
    }
    auto& qstate = qstate_it->second;
    fds_assert(qstate);

    /* update total min and avail rates */
    assert(qstate->assured_rate <= total_assured_rate);
    total_assured_rate -= qstate->assured_rate;

    new_total_min_rate = total_assured_rate;
    if (total_iops > total_assured_rate) {
        new_total_avail_rate = total_iops - total_assured_rate;
    }

    avail_pool.modifyRate(new_total_avail_rate);
    qstate_map.erase(qstate_it);
    qda_lock.write_unlock();

    LOGNOTIFY << "QosHTBDispatcher: deregistered queue 0x" << std::hex << queue_id << std::dec
              << "; total_min_rate " << new_total_min_rate
              << ", total_avail_rate " << new_total_avail_rate;

    return err;
}

void
QoSHTBDispatcher::setQueueThrottleLevel(queue_state_type& qstate, float const throttle_level)
{
    assert((throttle_level >= -10) && (throttle_level <= 11));
    fds_int32_t tlevel_x = (fds_int32_t) throttle_level;
    double tlevel_frac = fabs((double)throttle_level - (double)tlevel_x);
    setQueueThrottleLevel(qstate, tlevel_x, tlevel_frac);
}

void
QoSHTBDispatcher::setQueueThrottleLevel(queue_state_type& qstate, fds_int32_t const tlevel_x, double const tlevel_frac)
{
    fds_assert(qstate);
    if (tlevel_x >= 0) {
        auto tlevel_x_u = static_cast<fds_uint32_t>(tlevel_x);
        if (qstate->priority > tlevel_x_u) {
            /* throttle to assured rate */
            qstate->setEffectiveMinMaxRates(qstate->assured_rate,
                                            qstate->assured_rate,
                                            wait_time_microsec);
        } else if (qstate->priority < tlevel_x_u) {
            /* ok to go up to throttle limit */
            qstate->setEffectiveMinMaxRates(qstate->assured_rate, qstate->throttle_rate, wait_time_microsec);
        } else {
            /* ok to go up to assured_rate + Y/10*(throttle_rate - assured_rate) */
            fds_int64_t new_throttle_rate = qstate->assured_rate
                                            + tlevel_frac
                                            * (qstate->throttle_rate - qstate->assured_rate);
            qstate->setEffectiveMinMaxRates(qstate->assured_rate,
                                            new_throttle_rate,
                                            wait_time_microsec);
        }
    } else { /* tlevel_x < 0 */
        double share = (10.0 - (double)abs(tlevel_x))/10.0;
        fds_int64_t new_assured_rate = static_cast<fds_int64_t>(share * qstate->assured_rate);
        /* we don't want to go to absolute 0 rate, so set some very small rate as assured */
        if (new_assured_rate < HTB_QUEUE_RATE_MIN) {
            new_assured_rate = HTB_QUEUE_RATE_MIN;
        }

        qstate->setEffectiveMinMaxRates(new_assured_rate, new_assured_rate, wait_time_microsec);
        /* note that we are not going to change total_min_rate when changing queue's effective
         * min rate (meaning we are not increasing total available rate), because in this case
         * all volumes are throttled down, and no sharing is happening */
    }

    LOGNOTIFY << "QosHTBDispatcher: setThrottleLevel(X="
              << tlevel_x <<", Y/10=" << tlevel_frac
              << ") queue " << qstate->queue_id
              << " Policy (" << qstate->assured_rate
              << "," << qstate->throttle_rate << "," << qstate->priority
              << "), effective min_rate=" << qstate->getEffectiveMinRate()
              << ", effective max_rate=" << qstate->getEffectiveMaxRate();

}

void
QoSHTBDispatcher::setThrottleLevel(float throttle_level)
{
    assert((throttle_level >= -10) && (throttle_level <= 10));
    fds_int32_t tlevel_x = (fds_int32_t) throttle_level;
    double tlevel_frac = fabs((double)throttle_level - (double)tlevel_x);

    LOGNOTIFY << "QosHTBDispatcher: set throttle level to "
              << throttle_level
              << "; X=" << tlevel_x
              << ", Y/10=" << tlevel_frac;

    WriteGuard wg(qda_lock);
    for (auto& q_pair : qstate_map) {
        setQueueThrottleLevel(q_pair.second, tlevel_x, tlevel_frac);
    }
    current_throttle_level = throttle_level;
}

void
QoSHTBDispatcher::ioProcessForEnqueue(fds_qid_t queue_id,
        FDS_IOType *io)
{
    auto& qstate = qstate_map[queue_id];
    fds_assert(qstate);
    fds_uint32_t queued_ios = qstate->handleIoEnqueue(io);
    LOGDEBUG << "QoSHTBDispatcher: handling enqueue IO to queue 0x"
             << std::hex << queue_id << std::dec
             << " ; # of queued_ios " << (queued_ios+1);
}

void
QoSHTBDispatcher::ioProcessForDispatch(fds_qid_t queue_id,
        FDS_IOType *io)
{
    /* we already updated tokens in getNextQueueForDispatch(),
     * so only update number of queued ios  */
    auto& qstate = qstate_map[queue_id];
    fds_assert(qstate);
    qstate->handleIoDispatch(io);
}

/* find queue whose IO needs to be dispatched next */
fds_qid_t
QoSHTBDispatcher::getNextQueueForDispatch()
{
    fds_qid_t ret_qid = 0;
    TBQueueState *min_wma_qstate = nullptr;
    double min_wma {0.0};
    uint min_wma_hiprio {0};

    while (1) {

        /* reset qstate with min wma, since we start searching from beginning */
        min_wma_qstate = nullptr;

        /* all tokens are demand-driven, so make sure to update state first */
        fds_uint64_t nowMicrosec = util::getTimeStampMicros();
        avail_pool.updateTBState(nowMicrosec);

        /**** search if any queues has IOs that need to be dispatched to meet min_iops ****/
        /* we will check the queue that we serviced last time last */
        auto it = qstate_map.find(last_dispatch_qid);

        for (auto i = qstate_map.size(); 0 < i; --i, ++it) {
            /* next queue */
            if (it == qstate_map.end()) {
                it = qstate_map.begin();
            }

            auto& qstate = it->second;
            fds_assert(qstate);

            /* before querying any state, update tokens */
            nowMicrosec = util::getTimeStampMicros();
            fds_uint64_t exp_assured_toks = qstate->updateTokens(nowMicrosec);
            if (exp_assured_toks > 0) {
                /* first put expired assured tokens to the avail_pool */
                avail_pool.addTokens(exp_assured_toks);
                LOGDEBUG << "QoSHTVDispatcher: moving "
                         << exp_assured_toks << " expired assured toks from "
                         << "queue 0x" << std::hex << qstate->queue_id
                         << std::dec << " to the pool of available tokens";
            }

            /* try to see if we can serve the io from the head of queue with assured tokens */
            TBQueueState::tbStateType state = qstate->tryToConsumeAssuredTokens(1);
            if (state == TBQueueState::TBQUEUE_STATE_OK) {
                /* we found a queue whose IO we will dispatch to meet its min_ios */
                last_dispatch_qid = it->first;
                LOGDEBUG << "QoSHTBDispatcher: dispatch (min_iops) io from queue 0x"
                         << std::hex << it->first << std::dec;
                return it->first;
            } else if (state == TBQueueState::TBQUEUE_STATE_NO_ASSURED_TOKENS) {
                /* queue has at least one io and available tokens (but no assured tokens) */
                double q_wma = qstate->getIOPerfWMA();
                if (!min_wma_qstate) {
                    min_wma = q_wma;
                    min_wma_qstate = qstate.get();
                    min_wma_hiprio = qstate->priority;
                } else if (qstate->priority < min_wma_hiprio) {
                    /* assuming higher priority is a lower the number (highest = 1) */
                    min_wma_hiprio = qstate->priority;
                    min_wma = q_wma;
                    min_wma_qstate = qstate.get();
                } else if ((qstate->priority == min_wma_hiprio) && (q_wma < min_wma)) {
                    min_wma = q_wma;
                    min_wma_qstate = qstate.get();
                }
            }
        }

        /* we did not find any queue with IOs that need to meet it min_iops */
        if (min_wma_qstate) {
            /* we found queue that has IO, available tokens, and lowest recent ave performance */
            /* dispatch this io if we have available tokens in the resource pool */
            fds_bool_t bHasToks = avail_pool.tryToConsumeTokens(1);
            if (bHasToks) {
                min_wma_qstate->consumeTokens(1);
                ret_qid = min_wma_qstate->queue_id;
                LOGDEBUG << "QoSHTBDispatcher: dispatch (avail) io from queue 0x"
                    << std::hex << ret_qid << std::dec;
                break;
            }
        }

        /* we did not find any IOs to dispatch */
        { /* wait for next guaranteed token or few non-guaranteed tokens to be created */
            const double num_toks = 1.0;
            fds_uint64_t assured_delay_microsec = (total_assured_rate > 0) ? (num_toks/(double)total_assured_rate)*1000000.0 : 0;
            fds_uint64_t delay_microsec = (avail_pool.getRate() > 0) ? ((num_toks+1.0)/(double)avail_pool.getRate())*1000000.0 : 0;
            if ((assured_delay_microsec > 0) && (delay_microsec > assured_delay_microsec))
                delay_microsec = assured_delay_microsec;

            LOGDEBUG << "QoSHTBDispatcher: no tokens available, will sleep for " << delay_microsec;
            if (delay_microsec > 0) {
                qda_lock.read_unlock();
                boost::this_thread::sleep(boost::posix_time::microseconds(delay_microsec));
                qda_lock.read_lock();
            }
        }

    } // while (1)

    last_dispatch_qid = ret_qid;
    return ret_qid;
}

/******* TBQueueState implementation ***********/
TBQueueState::TBQueueState(fds_qid_t _queue_id,
        fds_int64_t _assured_rate,
        fds_int64_t _throttle_rate,
        fds_uint32_t _priority,
        fds_uint64_t _assured_wait_microsec,
        fds_uint64_t _burst_size)
    : queue_id(_queue_id),
    assured_rate(_assured_rate),
    throttle_rate(_throttle_rate),
    priority(_priority),
    tb_assured(_assured_rate, _burst_size, _assured_wait_microsec),
    tb_throttle(_throttle_rate, _burst_size)
{
    assert(_assured_rate <= _throttle_rate);
    queued_io_counter = ATOMIC_VAR_INIT(0);
    memset(recent_iops, 0, sizeof(fds_uint32_t) * HTB_WMA_LENGTH);

    /* align next_hist_ts to a second boundary, so that all volumes histories are aligned */
    next_hist_ts = util::getTimeStampMicros() + HTB_WMA_SLOT_SIZE_MICROSEC;
    hist_slotix = 0;
}

TBQueueState::~TBQueueState()
{
}

/* for now assuming constant IO cost, so simple implementation  */
fds_uint32_t
TBQueueState::handleIoEnqueue(FDS_IOType* /*io*/)
{
    return (queued_io_counter.fetch_add(1, std::memory_order_relaxed) + 1);
}

/* for now assuming constant IO cost */
fds_uint32_t
TBQueueState::handleIoDispatch(FDS_IOType* /*io*/)
{
    /* update recent iops performance */
    /* since this implementation assumes single dispatcher thread, each IO
     * dispatch happens one after another, if that assumption does not hold anymore
     * will need to change wma calculation  */
    fds_uint64_t now = util::getTimeStampMicros();
    if (now < next_hist_ts) {
        /* we are still filling the same slot */
        recent_iops[hist_slotix]++;
    } else {
        /* will start the next slot, possibly skipping idle */
        do {
            next_hist_ts += HTB_WMA_SLOT_SIZE_MICROSEC;
            hist_slotix = (hist_slotix + 1) % HTB_WMA_LENGTH;
            recent_iops[hist_slotix] = 0;
        } while (now > next_hist_ts);
        recent_iops[hist_slotix]++;
    }

    return (queued_io_counter.fetch_sub(1, std::memory_order_relaxed) - 1);
}

/* This is an array of powers of 0.9 */
const double TBQueueState::kweights[HTB_MAX_WMA_LENGTH] =
{1.0000, 0.9000, 0.8100, 0.7290, 0.6561, 0.5905, 0.5314, 0.4783, 0.4305, 0.3874,
 0.3487, 0.3138, 0.2824, 0.2541, 0.2288, 0.2059, 0.1853, 0.1668, 0.1501, 0.1351};
double
TBQueueState::getIOPerfWMA()
{
    /* calculating weighted average over recent history using
     * formula from Zygaria paper [RTAS 2006] */
    double wma = 0.0;
    int ix = (hist_slotix + 1) % HTB_WMA_LENGTH; // oldest history slot
    for (int i = (HTB_WMA_LENGTH-1); i >= 0; --i)
    {
        wma += (recent_iops[ix] * kweights[i]);
        ix = (ix + 1) % HTB_WMA_LENGTH;
    }
    return wma;
}

/* Updates both token buckets and returns number of expired 'assured' tokens */
fds_uint64_t TBQueueState::updateTokens(fds_uint64_t nowMicrosec)
{
    /* TODO: we can make it more efficient by getting the current time here
     * and adding token bucket methods that take current time (since
     * get time seems quite expensive), will do later, we may anyway try
     * to use chrono nanosec timers (cannot compile for some reason). */
    moveToNextHistTs(nowMicrosec);

    tb_throttle.updateTBState(nowMicrosec);
    return tb_assured.updateTBState(nowMicrosec);
}

/* will consume 'io_cost' tokens from tb_min if they are available, there is at
 * least one queued IO, and we are not over the max rate limit, otherwise
 * return one of the states */
TBQueueState::tbStateType TBQueueState::tryToConsumeAssuredTokens(fds_uint32_t io_cost)
{
    unsigned int queued_ios = queued_io_counter.load(std::memory_order_relaxed);
    if (queued_ios == 0) {
        return TBQUEUE_STATE_QUEUE_EMPTY;
    }

    /* check if we reached throttle_rate */
    if ( !tb_throttle.hasTokens(io_cost) ) {
        return TBQUEUE_STATE_NO_TOKENS;
    }

    /* finally, try to consume tokens from tb_assured */
    if ( !tb_assured.tryToConsumeTokens(io_cost) ) {
        return TBQUEUE_STATE_NO_ASSURED_TOKENS;
    }

    /* we consumed to assured tokens */
    tb_throttle.tryToConsumeTokens(io_cost);
    /* it's expected that we should always have tokens in tb_max
     * when there are tokens in tb_min -- not asserting for now */
    return TBQUEUE_STATE_OK;
}


} // namespace fds
