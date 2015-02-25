/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_CATALOGSYNCRECV_H_
#define SOURCE_DATA_MGR_INCLUDE_CATALOGSYNCRECV_H_

#include <unordered_map>
#include <string>

#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <fds_module.h>
#include <util/Log.h>
#include <fds_process.h>
#include <concurrency/Mutex.h>
#include <DmIoReq.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {

    /**
     * Responsible for receiving forwarded catalog updates from source
     * DM and managing volume and shadow QoS queues when in sync state
     */
    class CatSyncReceiver {
  public:
        explicit CatSyncReceiver(DmIoReqHandler* dm_req_hdlr);
        ~CatSyncReceiver();

        /**
         * Per-volume volume meta receiver states, only exists when
         * sync or forwarding is in progress
         */
        typedef enum {
            VSYNC_RECV_SYNCING,        /* rsync is in progress */
            VSYNC_RECV_FWD_INPROG,     /* rsync finished, forwarding in progress */
            VSYNC_RECV_FWD_FINISHING   /* no incoming fwd updates, draining shadow que */
        } volSyncRecvState;

        /**
         * Notification that we will receive existing volume meta for
         * this volume from another DM. So, we create appropriate volume data
         * structures to track the progress of volume meta sync and receive forwarded
         * updates
         */
        Error startRecvVolmeta(fds_volid_t volid, FDS_VolumeQueue* shadowQueue);

        /**
         * Called when rsyncs for volume is finished and we should start
         * processing forwarded updates
         */
        void startProcessFwdUpdates(fds_volid_t volid);

        /**
         * Called when source DM notifies us that it finished forwarding
         * updates
         */
        void handleFwdDone(fds_volid_t volid);

        /**
         * Enqueue forwarded update into volume's shadow queue
         */
        Error enqueueFwdUpdate(DmIoFwdCat* fwdReq);

        /**
         * WARNING: when this code was written, volume uuid only used low 63 bits,
         * so we assign shadow vol uuid by setting the most significant bit to 1;
         * if this assumption changes, our shadow volume id may not be unique!!!
         * so register volume may fail
         */
        inline fds_volid_t shadowVolUuid(fds_volid_t volid) const {
            return (volid | 0x8000000000000000);
        }

  private:  // methods
        struct VolReceiver {
            VolReceiver(fds_volid_t volid,
                        FDS_VolumeQueue* shadowQueue);
            ~VolReceiver();

            volSyncRecvState state;
            FDS_VolumeQueue* shadowVolQueue;
            fds_uint32_t pending;  // queued + outstanding
        };
        typedef boost::shared_ptr<VolReceiver> VolReceiverPtr;
        typedef std::unordered_map<fds_volid_t, VolReceiverPtr> VolToReceiverTable;

  private:  // members
        /**
         * Map of per-volume receivers that holds states and shadow qos queues
         * A volume receiver object only exists while rsync and forwarding for
         * this volume is in progress.
         */
        VolToReceiverTable vol_recv_map;
        fds_mutex cat_recv_lock;  // protects vol_recv_map

        /**
         * Pointer to DmIoReqHandler so we can queue requests to
         * shadow volume QoS queues; passed in constructor; does not own.
         */
        DmIoReqHandler *dm_req_handler;
    };

    typedef boost::shared_ptr<CatSyncReceiver> CatSyncReceiverPtr;

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_CATALOGSYNCRECV_H_
