/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <AmProcessor.h>

#include <algorithm>
#include <deque>
#include <string>

#include "fds_process.h"
#include "fds_timer.h"
#include <ObjectId.h>
#include "FdsRandom.h"
#include <fiu-control.h>
#include <util/fiu_util.h>
#include "AmDispatcher.h"
#include "AmTxManager.h"
#include "AmVolume.h"
#include "AmVolumeTable.h"
#include "AmVolumeAccessToken.h"

#include "requests/requests.h"
#include "AsyncResponseHandlers.h"

namespace fds {

#define AMPROCESSOR_CB_HANDLER(func, ...) \
    std::bind(&func, this, ##__VA_ARGS__ , std::placeholders::_1)

/*
 * Atomic for request ids.
 */
std::atomic_uint nextIoReqId;

/**
 * AM request processing layer. The processor handles state and
 * execution for AM requests.
 */
class AmProcessor_impl
{
    using shutdown_cb_type = std::function<void(void)>;

  public:
    AmProcessor_impl(Module* parent, CommonModuleProviderIf *modProvider)
        : amDispatcher(new AmDispatcher(modProvider)),
          txMgr(new AmTxManager()),
          volTable(nullptr),
          parent_mod(parent),
          have_tables(std::make_pair(false, false)),
          prepareForShutdownCb(nullptr)
    { }

    AmProcessor_impl(AmProcessor_impl const&) = delete;
    AmProcessor_impl& operator=(AmProcessor_impl const&) = delete;
    ~AmProcessor_impl() = default;

    void start();

    void prepareForShutdownMsgRespBindCb(shutdown_cb_type&& cb)
        { prepareForShutdownCb = cb; }

    void prepareForShutdownMsgRespCallCb();

    bool stop();

    Error enqueueRequest(AmRequest* amReq);

    Error modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc) {
        return volTable->modifyVolumePolicy(vol_uuid, vdesc);
    }

    void registerVolume(const VolumeDesc& volDesc)
        { volTable->registerVolume(volDesc); }

    void renewToken(const fds_volid_t vol_id);

    Error removeVolume(const VolumeDesc& volDesc);

    Error updateQoS(long int const* rate, float const* throttle)
        { return volTable->updateQoS(rate, throttle); }

    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);

    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);

    bool haveTables();

    bool isShuttingDown() const
    { SCOPEDREAD(shut_down_lock); return shut_down; }

  private:
    /// Unique ptr to the dispatcher layer
    std::unique_ptr<AmDispatcher> amDispatcher;

    /// Unique ptr to the transaction manager
    std::unique_ptr<AmTxManager> txMgr;

    /// Unique ptr to the volume table
    std::unique_ptr<AmVolumeTable> volTable;

    /// Unique ptr to a random num generator for tx IDs
    std::unique_ptr<RandNumGenerator> randNumGen;

    FdsTimer token_timer;

    Module* parent_mod {nullptr};

    std::pair<bool, bool> have_tables;

    bool shut_down { false };
    mutable fds_rwlock shut_down_lock;

    shutdown_cb_type prepareForShutdownCb;

    void processBlobReq(AmRequest *amReq);

    std::shared_ptr<AmVolume> getVolume(AmRequest* amReq, bool const allow_snapshot=true);
    inline bool haveCacheToken(std::shared_ptr<AmVolume> const& volume) const;
    inline bool haveWriteToken(std::shared_ptr<AmVolume> const& volume) const;

    /**
     * FEATURE TOGGLE: Single AM Enforcement
     * Wed 01 Apr 2015 01:52:55 PM PDT
     */
    bool volume_open_support { true };
    /**
     * FEATURE TOGGLE: Safe UpdateBlobOnce
     * Wed 19 Aug 2015 10:56:46 AM MDT
     */
    bool safe_atomic_write { false };
    std::chrono::duration<fds_uint32_t> vol_tok_renewal_freq {30};

    /**
     * Processes a get volume metadata request
     */
    void getVolumeMetadata(AmRequest *amReq);

    /**
     * Attachment request, retrieve volume descriptor
     */
    void attachVolume(AmRequest *amReq);
    void attachVolumeCb(AmRequest *amReq, const Error& error);
    void detachVolume(AmRequest *amReq);

    /**
     * Processes a abort blob transaction
     */
    void abortBlobTx(AmRequest *amReq);
    void abortBlobTxCb(AmRequest *amReq, const Error &error);

    /**
     * Processes a commit blob transaction
     */
    void commitBlobTx(AmRequest *amReq);
    void commitBlobTxCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a delete blob request
     */
    void deleteBlob(AmRequest *amReq);

    /**
     * Processes a get blob request
     */
    void getBlob(AmRequest *amReq);
    void getBlobCb(AmRequest *amReq, const Error& error);

    /**
     * Look for object data in SM
     */
    void getObject(AmRequest *amReq);

    /**
     * Processes a put blob request
     */
    void putBlob(AmRequest *amReq);
    void putBlobCb(AmRequest *amReq, const Error& error);
    void putBlobOnceCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a set volume metadata request
     */
    void setVolumeMetadata(AmRequest *amReq);

    /**
     * Processes a start blob transaction
     */
    void startBlobTx(AmRequest *amReq);
    void startBlobTxCb(AmRequest *amReq, const Error &error);

    /**
     * Processes a stat volume request
     */
    void statVolume(AmRequest *amReq);
    void statVolumeCb(AmRequest *amReq, const Error &error);

    /**
     * Callback for catalog query request
     */
    void queryCatalogCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a rename blob request
     */
    void renameBlob(AmRequest *amReq);
    void renameBlobCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a set metadata on blob request
     */
    void setBlobMetadata(AmRequest *amReq);

    /**
     * Processes a stat blob request
     */
    void statBlob(AmRequest *amReq);
    void statBlobCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a volumeContents (aka ListBucket) request
     */
    void volumeContents(AmRequest *amReq);

    /**
     * Generic callback for a few responses
     */
    void respond_and_delete(AmRequest *amReq, const Error& error)
        { if (respond(amReq, error).ok()) { delete amReq; } }

    Error respond(AmRequest *amReq, const Error& error);

};

Error
AmProcessor_impl::enqueueRequest(AmRequest* amReq) {
    static fpi::VolumeAccessMode const default_access_mode;

    Error err;
    {
        SCOPEDREAD(shut_down_lock);
        if (shut_down) {
            err = ERR_SHUTTING_DOWN;
        } else {
            amReq->io_req_id = nextIoReqId.fetch_add(1, std::memory_order_relaxed);
            err = volTable->enqueueRequest(amReq);
            // Volume Table now knows there's an outstanding request ok to
            // unlock the shutdown lock
        }
    }

    /** Queue and dispatch an attachment if we didn't find a volume */
    if (ERR_VOL_NOT_FOUND == err) {
        // TODO(bszmyd): Wed 27 May 2015 09:01:43 PM MDT
        // This code is here to support the fact that not all the connectors
        // send an AttachVolume currently. For now ensure one is enqueued in
        // the wait list by queuing a no-op attach request ourselves, this
        // will cause the attach to use the default mode.
        if (FDS_ATTACH_VOL != amReq->io_type) {
            auto attachReq = new AttachVolumeReq(invalid_vol_id,
                                                 amReq->volume_name,
                                                 default_access_mode,
                                                 nullptr);
            attachReq->io_req_id = nextIoReqId.fetch_add(1, std::memory_order_relaxed);
            volTable->enqueueRequest(attachReq);
        }
        err = amDispatcher->attachVolume(amReq->volume_name);
        if (ERR_NOT_READY == err) {
            // We don't have domain tables yet...just reject.
            volTable->removeVolume(amReq->volume_name, invalid_vol_id);
            return err;
        }
    }

    if (!err.ok()) {
        respond_and_delete(amReq, err);
    }
    return err;
}

void
AmProcessor_impl::processBlobReq(AmRequest *amReq) {
    fds_assert(amReq->io_module == FDS_IOType::ACCESS_MGR_IO);
    fds_assert(amReq->isCompleted() == true);

    switch (amReq->io_type) {
        /* === Volume operations === */
        case fds::FDS_ATTACH_VOL:
            attachVolume(amReq);
            break;

        case fds::FDS_DETACH_VOL:
            detachVolume(amReq);
            break;

        case fds::FDS_GET_VOLUME_METADATA:
            getVolumeMetadata(amReq);
            break;

        case fds::FDS_SET_VOLUME_METADATA:
            setVolumeMetadata(amReq);
            break;

        case fds::FDS_STAT_VOLUME:
            statVolume(amReq);
            break;

        case fds::FDS_VOLUME_CONTENTS:
            volumeContents(amReq);
            break;


        /* == Tx based operations == */
        case fds::FDS_ABORT_BLOB_TX:
            abortBlobTx(amReq);
            break;

        case fds::FDS_COMMIT_BLOB_TX:
            commitBlobTx(amReq);
            break;

        case fds::FDS_DELETE_BLOB:
            deleteBlob(amReq);
            break;

        case fds::FDS_PUT_BLOB:
            putBlob(amReq);
            break;

        case fds::FDS_SET_BLOB_METADATA:
            setBlobMetadata(amReq);
            break;

        case fds::FDS_START_BLOB_TX:
            startBlobTx(amReq);
            break;


        /* ==== Read operations ==== */
        case fds::FDS_GET_BLOB:
            getBlob(amReq);
            break;

        case fds::FDS_STAT_BLOB:
            statBlob(amReq);
            break;

        case fds::FDS_SM_GET_OBJECT:
            getObject(amReq);
            break;

        /* ==== Atomic operations === */
        case fds::FDS_PUT_BLOB_ONCE:
            // We piggy back this operation with some runtime conditions
            putBlob(amReq);
            break;

        case fds::FDS_RENAME_BLOB:
            renameBlob(amReq);
            break;

        default :
            LOGCRITICAL << "unimplemented request: " << amReq->io_type;
            respond_and_delete(amReq, ERR_NOT_IMPLEMENTED);
            break;
    }
}

void
AmProcessor_impl::start()
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    if (conf.get<fds_bool_t>("testing.uturn_processor_all")) {
        fiu_enable("am.uturn.processor.*", 1, NULL, 0);
    }
    auto qos_threads = conf.get<int>("qos_threads");

    FdsConfigAccessor features(g_fdsprocess->get_fds_config(), "fds.feature_toggle.");
    safe_atomic_write = features.get<bool>("am.safe_atomic_write", safe_atomic_write);
    volume_open_support = features.get<bool>("common.volume_open_support", volume_open_support);
    vol_tok_renewal_freq =
        std::chrono::duration<fds_uint32_t>(conf.get<fds_uint32_t>("token_renewal_freq"));

    LOGNORMAL << "Features: safe_atomic_write(" << safe_atomic_write
              << ") volume_open_support("       << volume_open_support << ")";

    randNumGen = RandNumGenerator::unique_ptr(
        new RandNumGenerator(RandNumGenerator::getRandSeed()));
    amDispatcher->start();

    auto closure = [this](AmRequest* amReq) mutable -> void { this->processBlobReq(amReq); };

    txMgr->init(closure);
    volTable.reset(new AmVolumeTable(qos_threads, GetLog()));
    volTable->registerCallback(closure);
}

Error
AmProcessor_impl::respond(AmRequest *amReq, const Error& error) {
    // markIODone will return ERR_DUPLICATE if the request has already
    // been responded to, in that case drop on the floor.
    Error err = volTable->markIODone(amReq);
    if (err.ok() && amReq->cb) {
        fpi::ErrorCode code {fpi::OK};
        if (!error.ok()) {
            switch (error.GetErrno()) {
                case ERR_DUPLICATE:
                case ERR_HASH_COLLISION:
                case ERR_VOL_DUPLICATE:
                    code = fpi::RESOURCE_ALREADY_EXISTS;
                    break;;
                case ERR_NOT_FOUND:
                case ERR_BLOB_NOT_FOUND:
                case ERR_CAT_ENTRY_NOT_FOUND:
                case ERR_VOL_NOT_FOUND:
                    code = fpi::MISSING_RESOURCE;
                    break;;
                case ERR_VOLUME_ACCESS_DENIED:
                    code = fpi::SERVICE_NOT_READY;
                    break;;
                default:
                    code = fpi::BAD_REQUEST;
                    break;;
            }
        }
        amReq->cb->call(code);
    }

    // If we're shutting down check if the
    // queue is empty and make the callback
    if (isShuttingDown()) {
        stop();
    }
    return err;
}

void AmProcessor_impl::prepareForShutdownMsgRespCallCb() {
    if (prepareForShutdownCb) {
        prepareForShutdownCb();
        prepareForShutdownCb = nullptr;
    }
}

bool AmProcessor_impl::stop() {
    SCOPEDWRITE(shut_down_lock);

    if (!shut_down) {
        LOGNOTIFY << "AmProcessor received a stop request.";
        shut_down = true;
        // Stop all timers, we're not going to attach anymore
        token_timer.destroy();
    }

    if (volTable->drained()) {
        // Close all attached volumes before finishing shutdown
        for (auto const& vol : volTable->getVolumes()) {
          removeVolume(*vol->voldesc);
        }
        parent_mod->mod_shutdown();
        return true;
    }
    return false;
}

std::shared_ptr<AmVolume>
AmProcessor_impl::getVolume(AmRequest* amReq, bool const allow_snapshot) {
    // check if this is a snapshot
    auto vol = volTable->getVolume(amReq->io_vol_id);
    if (!vol) {
        LOGCRITICAL << "unable to get volume info for vol: " << amReq->io_vol_id;
        respond_and_delete(amReq, ERR_NOT_READY);
    } else if (!allow_snapshot && vol->voldesc->isSnapshot()) {
        LOGWARN << "txn on a snapshot is not allowed.";
        respond_and_delete(amReq, ERR_VOLUME_ACCESS_DENIED);
        vol = nullptr;
    }
    return vol;
}

bool
AmProcessor_impl::haveCacheToken(std::shared_ptr<AmVolume> const& volume) const {
    if (volume) {
        /**
         * FEATURE TOGGLE: Single AM Enforcement
         * Wed 01 Apr 2015 01:52:55 PM PDT
         */
        if (!volume_open_support || (invalid_vol_token != volume->getToken())) {
            return volume->getMode().second;
        }
    }
    return false;
}

bool
AmProcessor_impl::haveWriteToken(std::shared_ptr<AmVolume> const& volume) const {
    if (volume) {
        /**
         * FEATURE TOGGLE: Single AM Enforcement
         * Wed 01 Apr 2015 01:52:55 PM PDT
         */
        if (!volume_open_support || (invalid_vol_token != volume->getToken())) {
            return volume->getMode().first;
        } else if (amDispatcher->getNoNetwork()) {
            // This is for testing purposes only
            return true;
        }
    }
    return false;
}

void
AmProcessor_impl::renewToken(const fds_volid_t vol_id) {
    // Get the current volume and token
    auto vol = volTable->getVolume(vol_id);
    if (!vol) {
        LOGDEBUG << "Ignoring token renewal for unknown (detached?) volume: " << vol_id;
        return;
    }

    // Dispatch for a renewal to DM, update the token on success. Remove the
    // volume otherwise.
    auto amReq = new AttachVolumeReq(vol_id, "", vol->access_token->getMode(), nullptr);
    amReq->token = vol->getToken();
    enqueueRequest(amReq);
}

Error
AmProcessor_impl::removeVolume(const VolumeDesc& volDesc) {
    LOGNORMAL << "Removing volume: " << volDesc.name;
    Error err{ERR_OK};

    // Remove the volume from QoS/VolumeTable, this is
    // called to clear any waiting requests with an error and
    // remove the QoS allocations
    auto vol = volTable->removeVolume(volDesc);

    // If we had a token for a volume, give it back to DM
    if (vol && vol->access_token) {
        // If we had a cache token for this volume, close it
        fds_int64_t token = vol->getToken();
        if (token_timer.cancel(boost::dynamic_pointer_cast<FdsTimerTask>(vol->access_token))) {
            LOGDEBUG << "Canceled timer for token: 0x" << std::hex << token;
        } else {
            LOGWARN << "Failed to cancel timer, volume will re-attach: "
                    << volDesc.name << " using: 0x" << std::hex << token;
        }
        amDispatcher->dispatchCloseVolume(volDesc.volUUID, token);
    }

    // Remove the volume from the caches (if there is one)
    txMgr->removeVolume(volDesc);

    return err;
}

Error
AmProcessor_impl::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    // If we successfully update the dlt, have the parent do it's init check
    auto e = amDispatcher->updateDlt(dlt_type, dlt_data, cb);
    if (e.ok() && !have_tables.first) {
        have_tables.first = true;
        parent_mod->mod_enable_service();
    }
    return e;
}

Error
AmProcessor_impl::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    // If we successfully update the dmt, have the parent do it's init check
    auto e = amDispatcher->updateDmt(dmt_type, dmt_data, cb);
    if (e.ok() && !have_tables.second) {
        have_tables.second = true;
        parent_mod->mod_enable_service();
    }
    return e;
}

bool
AmProcessor_impl::haveTables() {
    if (!have_tables.first) {
        have_tables.first = amDispatcher->getDLT().ok();
    }
    if (!have_tables.second) {
        have_tables.second = amDispatcher->getDMT().ok();
    }
    return have_tables.first && have_tables.second;
}

void
AmProcessor_impl::statVolume(AmRequest *amReq) {
    fiu_do_on("am.uturn.processor.statVol",
              respond_and_delete(amReq, ERR_OK); \
              return;);

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::statVolumeCb, amReq);
    amDispatcher->dispatchStatVolume(amReq);
}

void
AmProcessor_impl::statVolumeCb(AmRequest *amReq, const Error &error) {
    StatVolumeCallback::ptr cb =
            SHARED_DYN_CAST(StatVolumeCallback, amReq->cb);
    auto volReq = static_cast<StatVolumeReq *>(amReq);
    cb->current_usage_bytes = volReq->size;
    cb->blob_count = volReq->blob_count;
    respond_and_delete(amReq, error);
}

void
AmProcessor_impl::setVolumeMetadata(AmRequest *amReq) {
    fiu_do_on("am.uturn.processor.setVolMetadata",
              respond_and_delete(amReq, ERR_OK); \
              return;);

    auto vol = getVolume(amReq, false);
    if (!vol) {
      return;
    } else if (!haveWriteToken(vol)) {
        respond_and_delete(amReq, ERR_VOLUME_ACCESS_DENIED);
        return;
    }
    auto volReq = static_cast<SetVolumeMetadataReq*>(amReq);
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::respond_and_delete, amReq);
    amDispatcher->dispatchSetVolumeMetadata(amReq);
}

void
AmProcessor_impl::getVolumeMetadata(AmRequest *amReq) {
    fiu_do_on("am.uturn.processor.getVolMetadata",
              respond_and_delete(amReq, ERR_OK); \
              return;);

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::respond_and_delete, amReq);
    amDispatcher->dispatchGetVolumeMetadata(amReq);
}

void
AmProcessor_impl::attachVolume(AmRequest *amReq) {
    // NOTE(bszmyd): Wed 27 May 2015 11:45:32 PM MDT
    // Not cross-connector safe...
    // Check if we already are attached so we can have a current token
    auto volReq = static_cast<AttachVolumeReq*>(amReq);
    auto vol = getVolume(amReq);
    if (vol && vol->access_token)
    {
        token_timer.cancel(boost::dynamic_pointer_cast<FdsTimerTask>(vol->access_token));
        volReq->token = vol->getToken();
    }
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::attachVolumeCb, amReq);

    /**
     * FEATURE TOGGLE: Single AM Enforcement
     * Wed 01 Apr 2015 01:52:55 PM PDT
     */
    LOGDEBUG << "Dispatching open volume with mode: cache(" << volReq->mode.can_cache
             << ") write(" << volReq->mode.can_write << ")";
    amDispatcher->dispatchOpenVolume(amReq);
}

void
AmProcessor_impl::attachVolumeCb(AmRequest* amReq, Error const& error) {
    auto volReq = static_cast<AttachVolumeReq*>(amReq);
    Error err {error};

    SCOPEDREAD(shut_down_lock);
    auto vol = getVolume(amReq);
    if (!vol) return;

    auto& vol_desc = *vol->voldesc;
    if (!shut_down && err.ok()) {
        GLOGDEBUG << "For volume: " << vol_desc.volUUID
                  << ", received access token: 0x" << std::hex << volReq->token;


        // If this is a new token, create a access token for the volume
        auto access_token = vol->access_token;
        if (!access_token) {
            access_token = boost::make_shared<AmVolumeAccessToken>(
                token_timer,
                volReq->mode,
                volReq->token,
                [this, vol_id = vol_desc.volUUID] () mutable -> void {
                this->renewToken(vol_id);
                });
            err = volTable->processAttach(vol_desc, access_token);
            // Create caches if we have a token
            txMgr->registerVolume(vol_desc, volReq->mode.can_cache);
        } else {
            access_token->setMode(volReq->mode);
            access_token->setToken(volReq->token);
        }

        if (err.ok()) {
            // Renew this token at a regular interval
            auto timer_task = boost::dynamic_pointer_cast<FdsTimerTask>(access_token);
            if (!token_timer.schedule(timer_task, vol_tok_renewal_freq))
                { LOGWARN << "Failed to schedule token renewal timer!"; }

            // If this is a real request, set the return data
            if (amReq->cb) {
                auto cb = SHARED_DYN_CAST(AttachCallback, amReq->cb);
                cb->volDesc = boost::make_shared<VolumeDesc>(vol_desc);
                cb->mode = boost::make_shared<fpi::VolumeAccessMode>(volReq->mode);
            }
        }
    }

    if (ERR_OK != err) {
        LOGNOTIFY << "Failed to open volume with mode: cache(" << volReq->mode.can_cache
                  << ") write(" << volReq->mode.can_write
                  << ") error(" << err << ")";
        // Flush the volume's wait queue and return errors for pending requests
        removeVolume(vol_desc);
    }
    respond_and_delete(amReq, err);
}


void
AmProcessor_impl::detachVolume(AmRequest *amReq) {
    // This really can not fail, we have to be attached to be here
    auto vol = getVolume(amReq);
    if (!vol) return;

    removeVolume(*vol->voldesc);
    respond_and_delete(amReq, ERR_OK);
}

void
AmProcessor_impl::abortBlobTx(AmRequest *amReq) {
    auto blobReq = static_cast<AbortBlobTxReq*>(amReq);
    auto err = txMgr->getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        respond_and_delete(amReq, err);
        return;
    }
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::abortBlobTxCb, amReq);
    amDispatcher->dispatchAbortBlobTx(amReq);
}

void
AmProcessor_impl::abortBlobTxCb(AmRequest *amReq, const Error &error) {
    AbortBlobTxReq *blobReq = static_cast<AbortBlobTxReq *>(amReq);
    if (ERR_OK != txMgr->abortTx(*(blobReq->tx_desc)))
        LOGWARN << "Transaction unknown";

    respond_and_delete(amReq, error);
}

void
AmProcessor_impl::startBlobTx(AmRequest *amReq) {
    fiu_do_on("am.uturn.processor.startBlobTx",
              respond_and_delete(amReq, ERR_OK); \
              return;);

    auto vol = getVolume(amReq, false);
    if (!vol) {
      return;
    } else if (!haveWriteToken(vol)) {
        respond_and_delete(amReq, ERR_VOLUME_ACCESS_DENIED);
        return;
    }

    // Generate a random transaction ID to use
    static_cast<StartBlobTxReq*>(amReq)->tx_desc =
        boost::make_shared<BlobTxId>(randNumGen->genNumSafe());
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::startBlobTxCb, amReq);

    amDispatcher->dispatchStartBlobTx(amReq);
}

void
AmProcessor_impl::startBlobTxCb(AmRequest *amReq, const Error &error) {
    StartBlobTxCallback::ptr cb = SHARED_DYN_CAST(StartBlobTxCallback,
                                                  amReq->cb);

    StartBlobTxReq *blobReq = static_cast<StartBlobTxReq *>(amReq);
    if (error.ok()) {
        // Update callback and record new open transaction
        cb->blobTxId  = *blobReq->tx_desc;
        fds_verify(txMgr->addTx(amReq->io_vol_id,
                                *blobReq->tx_desc,
                                blobReq->dmt_version,
                                amReq->getBlobName()) == ERR_OK);
    }

    respond_and_delete(amReq, error);
}

void
AmProcessor_impl::deleteBlob(AmRequest *amReq) {
    auto vol = getVolume(amReq, false);
    if (!vol) {
      return;
    } else if (!haveWriteToken(vol)) {
        respond_and_delete(amReq, ERR_VOLUME_ACCESS_DENIED);
        return;
    }

    DeleteBlobReq* blobReq = static_cast<DeleteBlobReq *>(amReq);
    LOGDEBUG    << " volume:" << amReq->io_vol_id
                << " blob:" << amReq->getBlobName()
                << " txn:" << blobReq->tx_desc;

    // Update the tx manager with the delete op
    auto err = txMgr->getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        respond_and_delete(amReq, err);
        return;
    }
    txMgr->updateTxOpType(*(blobReq->tx_desc), amReq->io_type);

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::respond_and_delete, amReq);
    amDispatcher->dispatchDeleteBlob(amReq);
}

void
AmProcessor_impl::putBlob(AmRequest *amReq) {
    fiu_do_on("am.uturn.processor.putBlob",
              respond_and_delete(amReq, ERR_OK); \
              return;);

    auto vol = getVolume(amReq, false);
    if (!vol) {
      return;
    } else if (!haveWriteToken(vol)) {
        respond_and_delete(amReq, ERR_VOLUME_ACCESS_DENIED);
        return;
    }

    // Convert the offset to use a Byte term instead of Object
    fds_uint32_t maxObjSize = vol->voldesc->maxObjSizeInBytes;
    amReq->blob_offset = (amReq->blob_offset * maxObjSize);

    // Use a stock object ID if the length is 0.
    PutBlobReq *blobReq = static_cast<PutBlobReq *>(amReq);
    if (amReq->data_len == 0) {
        blobReq->obj_id = ObjectID();
    } else {
        SCOPED_PERF_TRACEPOINT_CTX(amReq->hash_perf_ctx);
        blobReq->obj_id = ObjIdGen::genObjectId(blobReq->dataPtr->c_str(), amReq->data_len);
    }

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::putBlobCb, amReq);
    if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
        blobReq->setTxId(randNumGen->genNumSafe());
        /**
         * FEATURE TOGGLE: Update object store before making catalog update.
         * This models the transaction update since the commit comes last to DM.
         * Will more thank likely cause an increase in latency in the write path
         * Wed 19 Aug 2015 10:56:46 AM MDT
         */
        if (safe_atomic_write) {
            // Make data stable on object store prior to updating the catalog, this
            // means sending them in series rather than in parallel.
            amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::putBlobOnceCb, amReq);
            blobReq->setResponseCount(1);
        } else {
            amDispatcher->dispatchUpdateCatalogOnce(amReq);
        }
    } else {
        // Verify we have a dmt version (and transaction) for this update
        auto err = txMgr->getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
        if (!err.ok()) {
            respond_and_delete(amReq, err);
            return;
        }
        amDispatcher->dispatchUpdateCatalog(amReq);
    }

    // Either dispatch the put blob request or, if there's no data, just call
    // our callback handler now (NO-OP).
    amReq->data_len > 0 ? amDispatcher->dispatchPutObject(amReq) :
                          blobReq->notifyResponse(ERR_OK);
}

void
AmProcessor_impl::putBlobCb(AmRequest *amReq, const Error& error) {
    auto blobReq = static_cast<PutBlobReq *>(amReq);

    if (error.ok()) {
        auto tx_desc = blobReq->tx_desc;
        // Add the Tx to the manager if this an updateOnce
        if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
            fds_verify(txMgr->addTx(amReq->io_vol_id,
                                    *tx_desc,
                                    blobReq->dmt_version,
                                    amReq->getBlobName()) == ERR_OK);
            // Stage the transaction metadata changes
            fds_verify(txMgr->updateStagedBlobDesc(*tx_desc, blobReq->final_meta_data));
        }

        // Update the transaction manager with the stage offset update
        if (ERR_OK != txMgr->updateStagedBlobOffset(*tx_desc,
                                                    amReq->getBlobName(),
                                                    amReq->blob_offset,
                                                    blobReq->obj_id)) {
            // An abort or commit already caused the tx
            // to be cleaned up. Short-circuit
            LOGNOTIFY << "Response no longer has active transaction: " << tx_desc->getValue();
            delete amReq;
            return;
        }

        // Update the transaction manager with the staged object data
        if (amReq->data_len > 0) {
            fds_verify(txMgr->updateStagedBlobObject(*tx_desc,
                                                     blobReq->obj_id,
                                                     blobReq->dataPtr)
                   == ERR_OK);
        }

        if (amReq->io_type == FDS_PUT_BLOB_ONCE) {
            txMgr->commitTx(*tx_desc, blobReq->final_blob_size);
        }
    }

    respond_and_delete(amReq, error);
}

void
AmProcessor_impl::putBlobOnceCb(AmRequest *amReq, const Error& error) {
    if (!error.ok()) {
        // Skip the volume update, we couldn't even store the data
        return putBlobCb(amReq, error);
    }
    auto blobReq = static_cast<PutBlobReq *>(amReq);
    // Sending the update in a single request. Create transaction ID to
    // use for the single request
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::putBlobCb, amReq);
    blobReq->setResponseCount(1);
    amDispatcher->dispatchUpdateCatalogOnce(amReq);
}

void
AmProcessor_impl::getBlob(AmRequest *amReq) {
    fiu_do_on("am.uturn.processor.getBlob",
              respond_and_delete(amReq, ERR_OK); \
              return;);

    auto volId = amReq->io_vol_id;
    auto vol = getVolume(amReq);
    if (!vol) {
        LOGCRITICAL << "getBlob failed to get volume for vol " << volId;
        return;
    }

    // TODO(Anna) We are doing update catalog using absolute
    // offsets, so we need to be consistent in query catalog
    // Review this!
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);
    auto const maxObjSize = vol->voldesc->maxObjSizeInBytes;
    blobReq->blob_offset = (amReq->blob_offset * maxObjSize);
    blobReq->blob_offset_end = blobReq->blob_offset;

    // If this is a large read, the number of end offset needs to encompass
    // the extra objects required.
    if (maxObjSize < amReq->data_len) {
        auto const extra_objects = (amReq->data_len / maxObjSize) - 1
                                 + ((0 != amReq->data_len % maxObjSize) ? 1 : 0);
        blobReq->blob_offset_end += extra_objects * maxObjSize;
    }

    // Create buffers for return objects, we don't know how many till we have
    // a valid descriptor
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);
    cb->return_buffers = boost::make_shared<std::vector<boost::shared_ptr<std::string>>>();

    // FIXME(bszmyd): Sun 26 Apr 2015 04:41:12 AM MDT
    // Don't support unaligned currently, reject if this is not
    if (0 != (amReq->blob_offset % maxObjSize)) {
        LOGERROR << "unaligned read not supported, offset: " << amReq->blob_offset
                 << " length: " << amReq->data_len;
        getBlobCb(amReq, ERR_INVALID);
        return;
    }

    // We can only read from the cache if we have an access token managing it
    Error err = ERR_OK;
    if (haveCacheToken(vol)) {
        // If we need to return metadata, check the cache
        if (blobReq->get_metadata) {
            auto cachedBlobDesc = txMgr->getBlobDescriptor(volId,
                                                           amReq->getBlobName(),
                                                           err);
            if (err.ok()) {
                LOGTRACE << "Found cached blob descriptor for " << std::hex
                         << volId << std::dec << " blob " << amReq->getBlobName();
                blobReq->metadata_cached = true;
                auto cb = SHARED_DYN_CAST(GetObjectWithMetadataCallback, amReq->cb);
                // Fill in the data here
                cb->blobDesc = cachedBlobDesc;
            }
        }

        // Check cache for object IDs
        if (err.ok()) {
            err = txMgr->getBlobOffsetObjects(volId,
                                              amReq->getBlobName(),
                                              amReq->blob_offset,
                                              amReq->blob_offset_end,
                                              maxObjSize,
                                              blobReq->object_ids);
        }

        // ObjectIDs were found in the cache
        if (err.ok() && (blobReq->metadata_cached == blobReq->get_metadata)) {
            // Found all metadata, just need object data
            blobReq->metadata_cached = true;
            amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::getBlobCb, amReq);
            return txMgr->getObjects(blobReq);
        }
    } else {
        LOGDEBUG << "Can't read from cache, dispatching to DM.";
    }

    // need to read from datamgr
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::queryCatalogCb, amReq);
    amDispatcher->dispatchQueryCatalog(amReq);
}

void AmProcessor_impl::getObject(AmRequest *amReq) {
    // Tx manager sets this request up including the proc_cb since it's
    // issuing them. Apparently we couldn't find the data in the cache
    amDispatcher->dispatchGetObject(amReq);
}

void
AmProcessor_impl::getBlobCb(AmRequest *amReq, const Error& error) {
    auto vol = getVolume(amReq);
    if (!vol) return;

    auto blobReq = static_cast<GetBlobReq *>(amReq);
    if (error == ERR_NOT_FOUND && !blobReq->retry) {
        // TODO(bszmyd): Mon 23 Mar 2015 02:40:25 AM PDT
        // This is somewhat of a trick since we don't really support
        // transactions. If we can't find the object, do an index lookup
        // again. If we get back the same ObjectID, then fine...try SM again,
        // but that's the end of it.
        blobReq->metadata_cached = false;
        blobReq->retry = true;
        blobReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::queryCatalogCb, amReq);
        GLOGDEBUG << "Dispatching retry on [ " << blobReq->volume_name
                  << ", " << blobReq->getBlobName()
                  << ", 0x" << std::hex << blobReq->blob_offset << std::dec
                  << "B ]";
        amDispatcher->dispatchQueryCatalog(amReq);
        return;
    } else if (ERR_OK == error) {
        // We still return all of the object even if less was requested, adjust
        // the return length, not the buffer.
        GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);

        // Calculate the sum size of our buffers
        size_t vector_size = std::accumulate(cb->return_buffers->cbegin(),
                                             cb->return_buffers->cend(),
                                             0,
                                             [] (size_t const& total_size, boost::shared_ptr<std::string> const& buf)
                                             { return total_size + buf->size(); });

        cb->return_size = std::min(amReq->data_len, vector_size);

        // If we have a cache token, we can stash this metadata
        if (!blobReq->metadata_cached && haveCacheToken(vol)) {
            txMgr->putOffsets(amReq->io_vol_id,
                              amReq->getBlobName(),
                              amReq->blob_offset,
                              vol->voldesc->maxObjSizeInBytes,
                              blobReq->object_ids);
            if (blobReq->get_metadata) {
                auto cbm = SHARED_DYN_CAST(GetObjectWithMetadataCallback, amReq->cb);
                if (cbm->blobDesc)
                    txMgr->putBlobDescriptor(amReq->io_vol_id,
                                               amReq->getBlobName(),
                                               cbm->blobDesc);
            }
        }
    }

    respond_and_delete(amReq, error);
}

void
AmProcessor_impl::renameBlob(AmRequest *amReq) {
    auto vol = getVolume(amReq, false);
    if (!vol) {
        return;
    } else if (!haveWriteToken(vol)) {
        respond_and_delete(amReq, ERR_VOLUME_ACCESS_DENIED);
        return;
    }

    auto blobReq = static_cast<RenameBlobReq*>(amReq);
    LOGDEBUG << "Renaming blob: " << blobReq->getBlobName()
             << " to: " << blobReq->new_blob_name;

    blobReq->tx_desc.reset(new BlobTxId(randNumGen->genNumSafe()));
    blobReq->dest_tx_desc.reset(new BlobTxId(randNumGen->genNumSafe()));
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::renameBlobCb, amReq);
    amDispatcher->dispatchRenameBlob(amReq);
}

void
AmProcessor_impl::renameBlobCb(AmRequest *amReq, const Error& error) {
    if (error.ok()) {
        auto blobReq = static_cast<RenameBlobReq*>(amReq);
        // TODO(bszmyd): Sun 02 Aug 2015 07:04:36 PM MDT
        // This is a gross nuke of the metadata since we don't remove the offset
        // caches for a blob. change this when we have a better interface to
        // the cache
        // Invalidate the old descriptors
        txMgr->invalidateMetaCache(amReq->io_vol_id);

        // Place the new blob's descriptor in the cache
        txMgr->putBlobDescriptor(blobReq->io_vol_id,
                                 blobReq->new_blob_name,
                                 SHARED_DYN_CAST(RenameBlobCallback, blobReq->cb)->blobDesc);
    }
    respond_and_delete(amReq, error);
}

void
AmProcessor_impl::setBlobMetadata(AmRequest *amReq) {
    auto vol = getVolume(amReq, false);
    if (!vol) {
        return;
    } else if (!haveWriteToken(vol)) {
        respond_and_delete(amReq, ERR_VOLUME_ACCESS_DENIED);
        return;
    }

    SetBlobMetaDataReq *blobReq = static_cast<SetBlobMetaDataReq *>(amReq);

    // Assert that we have a tx and dmt_version for the message
    auto err = txMgr->getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        respond_and_delete(amReq, err);
        return;
    }
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::respond_and_delete, amReq);

    amDispatcher->dispatchSetBlobMetadata(amReq);
}

void
AmProcessor_impl::statBlob(AmRequest *amReq) {
    fds_volid_t volId = amReq->io_vol_id;
    LOGDEBUG << "volume:" << volId <<" blob:" << amReq->getBlobName();

    auto vol = getVolume(amReq);
    if (!vol) return;
    auto can_cache = haveCacheToken(vol);

    if (can_cache) {
        // Check cache for blob descriptor
        Error err(ERR_OK);
        BlobDescriptor::ptr cachedBlobDesc = txMgr->getBlobDescriptor(volId,
                                                                        amReq->getBlobName(),
                                                                        err);
        if (ERR_OK == err) {
            LOGTRACE << "Found cached blob descriptor for " << std::hex
                << volId << std::dec << " blob " << amReq->getBlobName();

            StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, amReq->cb);
            // Fill in the data here
            cb->blobDesc = cachedBlobDesc;
            statBlobCb(amReq, ERR_OK);
            return;
        }
        LOGTRACE << "Did not find cached blob descriptor for " << std::hex
            << volId << std::dec << " blob " << amReq->getBlobName();
    }

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::statBlobCb, amReq);
    amDispatcher->dispatchStatBlob(amReq);
}

void
AmProcessor_impl::queryCatalogCb(AmRequest *amReq, const Error& error) {
    if (error == ERR_OK) {
        // We got the metadata, now get the objects
        amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::getBlobCb, amReq);
        return txMgr->getObjects(static_cast<GetBlobReq *>(amReq));
    } else {
        respond_and_delete(amReq, error);
    }
}

void
AmProcessor_impl::statBlobCb(AmRequest *amReq, const Error& error) {
    auto vol = getVolume(amReq);
    if (!vol) return;
    respond(amReq, error);

    // Insert metadata into cache.
    if (ERR_OK == error && haveCacheToken(vol)) {
        txMgr->putBlobDescriptor(amReq->io_vol_id,
                                   amReq->getBlobName(),
                                   SHARED_DYN_CAST(StatBlobCallback, amReq->cb)->blobDesc);
    }

    delete amReq;
}

void
AmProcessor_impl::volumeContents(AmRequest *amReq) {
    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::respond_and_delete, amReq);
    amDispatcher->dispatchVolumeContents(amReq);
}

void
AmProcessor_impl::commitBlobTx(AmRequest *amReq) {
    auto vol = getVolume(amReq, false);
    if (!vol) {
      return;
    } else if (!haveWriteToken(vol)) {
      respond_and_delete(amReq, ERR_VOLUME_ACCESS_DENIED);
      return;
    }
    auto blobReq = static_cast<CommitBlobTxReq*>(amReq);

    auto err = txMgr->getTxDmtVersion(*(blobReq->tx_desc), &(blobReq->dmt_version));
    if (!err.ok()) {
        respond_and_delete(amReq, err);
        return;
    }

    amReq->proc_cb = AMPROCESSOR_CB_HANDLER(AmProcessor_impl::commitBlobTxCb, amReq);
    amDispatcher->dispatchCommitBlobTx(amReq);
}

void
AmProcessor_impl::commitBlobTxCb(AmRequest *amReq, const Error &error) {
    // Push the committed update to the cache and remove from manager
    if (ERR_OK == error) {
        CommitBlobTxReq *blobReq = static_cast<CommitBlobTxReq *>(amReq);
        txMgr->updateStagedBlobDesc(*(blobReq->tx_desc), blobReq->final_meta_data);
        txMgr->commitTx(*(blobReq->tx_desc), blobReq->final_blob_size);
    } else {
        LOGERROR << "Transaction failed to commit: " << error;
    }

    respond_and_delete(amReq, error);
}

/**
 * Pimpl forwarding methods. Should just call the underlying implementaion
 */
AmProcessor::AmProcessor(Module* parent, CommonModuleProviderIf *modProvider)
        : enable_shared_from_this<AmProcessor>(),
          _impl(new AmProcessor_impl(parent, modProvider))
{ }

AmProcessor::~AmProcessor() = default;

void AmProcessor::start()
{ return _impl->start(); }

void AmProcessor::prepareForShutdownMsgRespBindCb(shutdown_cb_type&& cb)
{
    return _impl->prepareForShutdownMsgRespBindCb(std::move(cb));
}

void AmProcessor::prepareForShutdownMsgRespCallCb()
{
    return _impl->prepareForShutdownMsgRespCallCb();
}

bool AmProcessor::stop()
{ return _impl->stop(); }

Error AmProcessor::enqueueRequest(AmRequest* amReq)
{ return _impl->enqueueRequest(amReq); }

bool AmProcessor::haveTables()
{ return _impl->haveTables(); }

bool AmProcessor::isShuttingDown() const
{ return _impl->isShuttingDown(); }

Error AmProcessor::modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc)
{ return _impl->modifyVolumePolicy(vol_uuid, vdesc); }

void AmProcessor::registerVolume(const VolumeDesc& volDesc)
{ return _impl->registerVolume(volDesc); }

Error AmProcessor::removeVolume(const VolumeDesc& volDesc)
{ return _impl->removeVolume(volDesc); }

Error AmProcessor::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb)
{ return _impl->updateDlt(dlt_type, dlt_data, cb); }

Error AmProcessor::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb)
{ return _impl->updateDmt(dmt_type, dmt_data, cb); }

Error AmProcessor::updateQoS(long int const* rate, float const* throttle)
{ return _impl->updateQoS(rate, throttle); }

}  // namespace fds
