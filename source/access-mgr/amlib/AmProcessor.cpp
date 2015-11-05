/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */

#include <AmProcessor.h>

#include <algorithm>
#include <deque>
#include <string>

#include "fds_process.h"
#include "fds_timer.h"
#include <fiu-control.h>
#include <util/fiu_util.h>
#include "AmTxManager.h"
#include "AmVolume.h"
#include "AmVolumeTable.h"
#include "AmVolumeAccessToken.h"

#include "requests/requests.h"
#include "AsyncResponseHandlers.h"

namespace fds {

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
        : txMgr(new AmTxManager(modProvider)),
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
    /// Unique ptr to the transaction manager
    std::unique_ptr<AmTxManager> txMgr;

    /// Unique ptr to the volume table
    std::unique_ptr<AmVolumeTable> volTable;

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
     * Processes a commit blob transaction
     */
    void commitBlobTx(AmRequest *amReq);

    /**
     * Processes a delete blob request
     */
    void deleteBlob(AmRequest *amReq);

    /**
     * Processes a get blob request
     */
    void getBlob(AmRequest *amReq);

    /**
     * Processes a put blob request
     */
    void putBlob(AmRequest *amReq);

    /**
     * Processes a set volume metadata request
     */
    void setVolumeMetadata(AmRequest *amReq);

    /**
     * Processes a start blob transaction
     */
    void startBlobTx(AmRequest *amReq);

    /**
     * Processes a rename blob request
     */
    void renameBlob(AmRequest *amReq);

    /**
     * Processes a set metadata on blob request
     */
    void setBlobMetadata(AmRequest *amReq);

    /**
     * Processes a stat blob request
     */
    void statBlob(AmRequest *amReq);

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
        err = txMgr->attachVolume(amReq->volume_name);
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
            fiu_do_on("am.uturn.processor.statVol",
                      respond_and_delete(amReq, ERR_OK); \
                      return;);
            txMgr->statVolume(amReq);
            break;

        case fds::FDS_VOLUME_CONTENTS:
            txMgr->volumeContents(amReq);
            break;

        /* == Tx based operations == */
        case fds::FDS_ABORT_BLOB_TX:
            txMgr->abortBlobTx(amReq);
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
    auto safe_atomic_write = features.get<bool>("am.safe_atomic_write", false);
    volume_open_support = features.get<bool>("common.volume_open_support", volume_open_support);
    vol_tok_renewal_freq =
        std::chrono::duration<fds_uint32_t>(conf.get<fds_uint32_t>("token_renewal_freq"));

    LOGNORMAL << "Features: safe_atomic_write(" << safe_atomic_write
              << ") volume_open_support("       << volume_open_support << ")";

    auto proc_closure = [this](AmRequest* amReq) mutable -> void { this->processBlobReq(amReq); };

    auto cb_closure = [this](AmRequest* amReq, Error const& error) mutable -> void {
        this->respond_and_delete(amReq, error);
    };

    txMgr->init(safe_atomic_write, cb_closure);
    volTable.reset(new AmVolumeTable(qos_threads, GetLog()));
    volTable->registerCallback(proc_closure);
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
        } else if (txMgr->getNoNetwork()) {
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
        txMgr->closeVolume(volDesc.volUUID, token);
    }

    // Remove the volume from the caches (if there is one)
    txMgr->removeVolume(volDesc);

    return err;
}

Error
AmProcessor_impl::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    // If we successfully update the dlt, have the parent do it's init check
    auto e = txMgr->updateDlt(dlt_type, dlt_data, cb);
    if (e.ok() && !have_tables.first) {
        have_tables.first = true;
        parent_mod->mod_enable_service();
    }
    return e;
}

Error
AmProcessor_impl::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    // If we successfully update the dmt, have the parent do it's init check
    auto e = txMgr->updateDmt(dmt_type, dmt_data, cb);
    if (e.ok() && !have_tables.second) {
        have_tables.second = true;
        parent_mod->mod_enable_service();
    }
    return e;
}

bool
AmProcessor_impl::haveTables() {
    if (!have_tables.first) {
        have_tables.first = txMgr->getDLT().ok();
    }
    if (!have_tables.second) {
        have_tables.second =txMgr->getDMT().ok();
    }
    return have_tables.first && have_tables.second;
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
    txMgr->setVolumeMetadata(amReq);
}

void
AmProcessor_impl::getVolumeMetadata(AmRequest *amReq) {
    fiu_do_on("am.uturn.processor.getVolMetadata",
              respond_and_delete(amReq, ERR_OK); \
              return;);

    txMgr->getVolumeMetadata(amReq);
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
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        attachVolumeCb(amReq, error);
    };

    /**
     * FEATURE TOGGLE: Single AM Enforcement
     * Wed 01 Apr 2015 01:52:55 PM PDT
     */
    LOGDEBUG << "Dispatching open volume with mode: cache(" << volReq->mode.can_cache
             << ") write(" << volReq->mode.can_write << ")";
    txMgr->openVolume(amReq);
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

    txMgr->startBlobTx(amReq);
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
    txMgr->deleteBlob(amReq);
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
    amReq->object_size = vol->voldesc->maxObjSizeInBytes;
    txMgr->updateCatalog(amReq);
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
    blobReq->object_size = vol->voldesc->maxObjSizeInBytes;

    // We can only read from the cache if we have an access token managing it
    if (haveCacheToken(vol)) {
        blobReq->forced_unit_access = false;
        blobReq->page_out_cache = true;
    }

    txMgr->getBlob(amReq);
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

    txMgr->renameBlob(amReq);
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
    txMgr->setBlobMetadata(amReq);
}

void
AmProcessor_impl::statBlob(AmRequest *amReq) {
    fds_volid_t volId = amReq->io_vol_id;
    LOGDEBUG << "volume:" << volId <<" blob:" << amReq->getBlobName();

    auto vol = getVolume(amReq);
    if (!vol) return;
    if (haveCacheToken(vol)) {
        amReq->forced_unit_access = false;
        amReq->page_out_cache = true;
    }

    txMgr->statBlob(amReq);
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
    txMgr->commitBlobTx(amReq);
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
