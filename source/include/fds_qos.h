/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_QOS_H_
#define SOURCE_INCLUDE_FDS_QOS_H_

#include <fds_types.h>
#include <fds_error.h>
#include <util/Log.h>
#include <fds_process.h>
#include <concurrency/RwLock.h>
#include <concurrency/Mutex.h>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <util/timeutils.h>
#include "qos_ctrl.h"
#include "PerfTrace.h"


namespace fds {

    typedef std::unordered_map<fds_qid_t, FDS_VolumeQueue *> queue_map_t;
    class FDS_QoSControl;

    class FDS_QoSDispatcher {
      public:
        fds_log *qda_log;
        queue_map_t queue_map;
        FDS_QoSControl *parent_ctrlr;
        float current_throttle_level;
        fds_uint64_t total_svc_rate;
        fds_uint32_t max_outstanding_ios;
        fds_rwlock qda_lock;  // Protects queue_map (and any high level structures in derived class)
                              // from events like volumes being inserted or removed during
                              // enqueue IO or dispatchIO.

        // If atomic vars are placed in the same cache line, it creates a false-sharing
        // where the cache line can pin-ping between processors.  It's better to isolate
        // them to different cache line.  The current cacheline size in x86_64 is 128 bytes.
        alignas(128) std::atomic<unsigned int> num_pending_ios;
        alignas(128) std::atomic<unsigned int> num_outstanding_ios;

        fds_uint64_t num_ios_completed = 0;
        fds_uint64_t num_ios_dispatched = 0;

        std::atomic_bool shuttingDown;
        fds_bool_t bypass_dispatcher;

        virtual fds_qid_t getNextQueueForDispatch() = 0;


        FDS_QoSDispatcher() :
            shuttingDown(false)
        {
        }

        FDS_QoSDispatcher(FDS_QoSControl *ctrlr,
                          fds_log *log,
                          fds_uint64_t total_server_rate) :
        FDS_QoSDispatcher()
        {
            parent_ctrlr = ctrlr;
            qda_log = log;
            total_svc_rate = total_server_rate;
            max_outstanding_ios = 0;
            num_pending_ios = ATOMIC_VAR_INIT(0);
            num_outstanding_ios = ATOMIC_VAR_INIT(0);
            FdsConfigAccessor config(g_fdsprocess->get_conf_helper());
            bypass_dispatcher = config.get_abs<bool>("fds.disable_qos");
            LOGNOTIFY << "Will bypass QoS? " << bypass_dispatcher;
        }

        ~FDS_QoSDispatcher() {
            shuttingDown = true;
        }

        void stop()
        {
            shuttingDown = true;
        }

        Error registerQueueWithLockHeld(fds_qid_t queue_id, FDS_VolumeQueue *queue)
        {
            Error err(ERR_OK);

            if (queue_map.count(queue_id) != 0) {
                err = ERR_DUPLICATE;
                return err;
            }
            queue_map[queue_id] = queue;

            LOGNOTIFY << "Dispatcher: registering queue - 0x"
                      << std::hex << queue_id << std::dec << " with min - "
                      << queue->iops_min << ", max - " << queue->iops_max
                      << ", priority - " << queue->priority
                      << ", total server rate = " << total_svc_rate;

            return err;
        }

        virtual Error registerQueue(fds_qid_t queue_id, FDS_VolumeQueue *queue )
        {
            Error err(ERR_OK);
            qda_lock.write_lock();
            err = registerQueueWithLockHeld(queue_id, queue);
            qda_lock.write_unlock();
            return err;
        }


        virtual Error modifyQueueQosParams(fds_qid_t queue_id,
                                           fds_uint64_t iops_min,
                                           fds_uint64_t iops_max,
                                           fds_uint32_t prio)
        {
            Error err(ERR_OK);
            qda_lock.write_lock();
            err = modifyQueueQosWithLockHeld(queue_id, iops_min, iops_max, prio);
            qda_lock.write_unlock();
            return err;
        }

        Error modifyQueueQosWithLockHeld(fds_qid_t queue_id,
                                         fds_uint64_t iops_min,
                                         fds_uint64_t iops_max,
                                         fds_uint32_t prio)
        {
            Error err(ERR_OK);
            if (queue_map.count(queue_id) == 0) {
                err = Error(ERR_INVALID_ARG);
                return err;
            }

            FDS_VolumeQueue *que = queue_map[queue_id];

            LOGNOTIFY << "Dispatcher: modifying queue qos params- 0x"
                      << std::hex << queue_id << std::dec << " with min - "
                      << iops_min << ", max - " << iops_max
                      << ", priority - " << prio
                      << ", total server rate = " << total_svc_rate;

            que->modifyQosParams(iops_min, iops_max, prio);
            return err;
        }

        Error deregisterQueueWithLockHeld(fds_qid_t queue_id)
        {
            Error err(ERR_OK);
            if  (queue_map.count(queue_id) == 0) {
                err = Error(ERR_INVALID_ARG);
                return err;
            }
            / /FDS_VolumeQueue *que = queue_map[queue_id];
            // Assert here that queue is empty, when one such API is available.
            queue_map.erase(queue_id);
            return err;
        }

        virtual Error deregisterQueue(fds_qid_t queue_id)
        {
            Error err(ERR_OK);
            qda_lock.write_lock();
            err = deregisterQueueWithLockHeld(queue_id);
            qda_lock.write_unlock();
            return err;
        }

        // Assumes caller has the qda read lock
        virtual void ioProcessForEnqueue(fds_qid_t queue_id, FDS_IOType *io)
        {
            // preprocess before enqueuing in the input queue
            LOGDEBUG << "Request " << io->io_req_id << " being enqueued at queue " << queue_id;
        }

        // Assumes caller has the qda read lock
        virtual void ioProcessForDispatch(fds_qid_t queue_id, FDS_IOType *io)
        {
            // do necessary processing before dispatching the io
            LOGDEBUG << "Dispatching " << io->io_req_id << " from queue "
                     << std::hex << queue_id << std::dec;
        }

        // Quiesce queued IOs on this queue & block any new IOs
        virtual void quiesceIOs(fds_qid_t queue_id)
        {
            Error err(ERR_OK);
            if (queue_map.count(queue_id) == 0) {
                return;
            }
            FDS_VolumeQueue *que = queue_map[queue_id];
            que->quiesceIOs();
        }

        virtual void suspendQueue(fds_qid_t queue_id)
        {
            if (queue_map.count(queue_id) == 0) {
                return;
            }
            FDS_VolumeQueue *que = queue_map[queue_id];
            que->suspendIO();
        }

        virtual void deactivateQueue(fds_qid_t queue_id)
        {
            if (queue_map.count(queue_id) == 0) {
                return;
            }
            FDS_VolumeQueue *que = queue_map[queue_id];
            que->stopDequeue();
        }

        virtual void resumeQueue(fds_qid_t queue_id)
        {
            if (queue_map.count(queue_id) == 0) {
                return;
            }
            FDS_VolumeQueue *que = queue_map[queue_id];
            que->resumeIO();
        }

        virtual void setThrottleLevel(float throttle_level)
        {
            assert((throttle_level >= -10) && (throttle_level <= 10));
            current_throttle_level = throttle_level;
            LOGNORMAL << "Dispatcher adjusting current throttle level to " << throttle_level;
        }

        virtual fds_uint32_t count(fds_qid_t queue_id)
        {
            SCOPEDREAD(qda_lock);
            queue_map_t::iterator iter = queue_map.find(queue_id);
            return queue_map.end() != iter ?  iter->second->count() : 0;
        }

        virtual FDS_VolumeQueue* getQueue(fds_qid_t queue_id)
        {
            SCOPEDREAD(qda_lock);
            queue_map_t::iterator iter = queue_map.find(queue_id);
            return queue_map.end() != iter ? iter->second : 0;
        }

        virtual Error enqueueIO(fds_qid_t queue_id, FDS_IOType *io)
        {
            Error err(ERR_OK);
            fds_uint32_t n_pios = 0;

            io->enqueue_ts = util::getTimeStampNanos();

            qda_lock.read_lock();
            if (queue_map.count(queue_id) == 0) {
                qda_lock.read_unlock();
                return Error(ERR_VOL_NOT_FOUND);
            }

            PerfTracer::tracePointBegin(io->opQoSWaitCtx);

            if (bypass_dispatcher == false) {
                FDS_VolumeQueue *que = queue_map[queue_id];
                que->enqueueIO(io);

                ioProcessForEnqueue(queue_id, io);

                n_pios = atomic_fetch_add(&(num_pending_ios), (unsigned int)1);
                LOGDEBUG << "Dispatcher: enqueueIO at queue - 0x"
                         << std::hex << queue_id << std::dec
                         <<  " : # of pending ios = " << n_pios+1;
                assert(n_pios >= 0);
            }
            qda_lock.read_unlock();

            if (bypass_dispatcher == true) {
                parent_ctrlr->processIO(io);
            }
            return err;
        }

        void setSchedThreadPriority()
        {
            pthread_t this_thread = pthread_self();
            struct sched_param params;
            int ret = 0;

            params.sched_priority = sched_get_priority_max(SCHED_FIFO);
            LOGNOTIFY << "Dispatcher: trying to set dispatcher thread realtime prio = "
                      << params.sched_priority;

            /* Attempt to set thread real-time priority to the SCHED_FIFO policy */
            ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
            if (ret != 0) {
                LOGWARN << "Dispatcher: Unsuccessful in setting scheduler thread realtime prio";
            } else {
                /* success - Now verify the change in thread priority */
                int policy = 0;
                ret = pthread_getschedparam(this_thread, &policy, &params);
                if (ret != 0) {
                LOGNOTIFY << "Dispatcher: "
                          << "Couldn't retrieve real-time scheduling parameters"
                          << " for dispatcher thread";
            }

            /* Check the correct policy was applied */
            if (policy != SCHED_FIFO) {
                LOGWARN << "Dispatcher:Scheduling is NOT SCHED_FIFO!";
            } else {
                LOGNOTIFY << "Dispatcher: thread SCHED_FIFO OK";
            }

            /* Print thread scheduling priority */
            LOGNOTIFY << "Dispatcher: Scheduler thread priority is " << params.sched_priority;
            }
        }

        virtual Error dispatchIOs()
        {
            Error err(ERR_OK);

            /* setting sched thread high prio breaks write perf on high iops
            * not setting it works ok for both reads and writes; may need to revisit if
            * if want to set both scheduler and ice threads (that repond to incoming packets) high prio */
            // setSchedThreadPriority();

            while (1) {
                if (shuttingDown == true) {
                    break;
                }

                parent_ctrlr->waitForWorkers();

                fds_uint32_t n_pios = 0;
                while (1) {
                    // TODO(Sean):
                    // atomic read  in a tight look can potentially saturate the memory bus.
                    // May need to revisit this.
                    n_pios = atomic_load(&num_pending_ios);

                    if (shuttingDown == true) {
                        return err;
                    } else if (n_pios > 0) {
                        break;
                    }

                    if (bypass_dispatcher == false) {
                        boost::this_thread::sleep(boost::posix_time::microseconds(100));
                    } else {
                        boost::this_thread::sleep(boost::posix_time::microseconds(100000));
                    }
                }

                fds_uint32_t n_oios = 0;
                if (max_outstanding_ios > 0) {
                    while (1) {
                        // TODO(Sean):
                        // atomic read  in a tight look can potentially saturate the memory bus.
                        // May need to revisit this.
                        n_oios = atomic_load(&num_outstanding_ios);
                        if (n_oios < max_outstanding_ios) {
                            break;
                        }
                        boost::this_thread::sleep(boost::posix_time::microseconds(100));
                    }
                }

                qda_lock.read_lock();

                fds_qid_t queue_id = getNextQueueForDispatch();
                if (queue_id == 0) {
                    qda_lock.read_unlock();

                    // this can happen if there are pending IOs but
                    // they are only in inactive queues, so there are no queues
                    // to dispatch from

                    LOGDEBUG << "Dispatcher: All active queues empty, retry later";

                    boost::this_thread::sleep(boost::posix_time::microseconds(100));
                    continue;
                }

                FDS_VolumeQueue *que = queue_map[queue_id];

                FDS_IOType *io = que->dequeueIO();
                assert(io != NULL);

                ioProcessForDispatch(queue_id, io);

                qda_lock.read_unlock();

                io->dispatch_ts = util::getTimeStampNanos();

                n_pios = 0;
                n_pios = atomic_fetch_sub(&(num_pending_ios), (unsigned int)1);
                // assert(n_pios >= 1);

                n_oios = 0;
                n_oios = atomic_fetch_add(&(num_outstanding_ios), (unsigned int)1);

                LOGDEBUG << "Dispatcher: dispatchIO from queue 0x"
                         << std::hex << queue_id << std::dec
                         << " : # of outstanding ios = " << n_oios+1
                         << " : # of pending ios = " << n_pios-1;
                // assert(n_oios >= 0);

                parent_ctrlr->processIO(io);
            }

            return err;
        }

        virtual Error markIODone(FDS_IOType *io)
        {
            Error err(ERR_OK);
            fds_uint32_t n_oios = 1;

            if (bypass_dispatcher == false) {
                n_oios = atomic_fetch_sub(&(num_outstanding_ios), (unsigned int)1);
                if (max_outstanding_ios > 0) {
                    /*
                    * We shouldn't be going over max_outstanding_ios,
                    * but we might (oops...). We check that we're not
                    * too far over it though (a looser check).
                    */
                    fds_verify(n_oios < 2 * max_outstanding_ios);
                }
            }

            io->io_done_ts = util::getTimeStampNanos();
            fds_uint64_t wait_nano = io->dispatch_ts - io->enqueue_ts;
            fds_uint64_t service_nano = io->io_done_ts - io->dispatch_ts;
            fds_uint64_t total_nano = io->io_done_ts - io->enqueue_ts;

            io->io_wait_time = static_cast<double>(wait_nano) / 1000.0;
            io->io_service_time = static_cast<double>(service_nano) / 1000.0;
            io->io_total_time = static_cast<double>(total_nano) / 1000.0;

            LOGDEBUG << "Dispatcher: IO Request " << io->io_req_id
                   << " for vol id 0x" << std::hex << io->io_vol_id << std::dec
                   << " completed in " << io->io_service_time
                   << " usecs with a wait time of " << io->io_wait_time
                   << " usecs with total io time of " << io->io_total_time
                   << " usecs. # of outstanding ios = " << n_oios-1;

            return err;
        }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_QOS_H_
