/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */

#include <AmProcessor.h>

#include <algorithm>
#include <deque>
#include <string>

#include "fds_process.h"
#include "fds_table.h"
#include <fiu-control.h>
#include <util/fiu_util.h>
#include "AmVolume.h"
#include "AmQoSCtrl.h"

#include "requests/requests.h"
#include "AsyncResponseHandlers.h"

namespace fds {

/**
 * AM request processing layer. The processor handles state and
 * execution for AM requests.
 */
class AmProcessor_impl
{
    using shutdown_cb_type = std::function<void(void)>;
    CommonModuleProviderIf* provider;

  public:
    AmProcessor_impl(Module* parent, CommonModuleProviderIf *modProvider)
        : provider(modProvider),
          qos_ctrl(nullptr),
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

    Error modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc)
        { return qos_ctrl->modifyVolumePolicy(vol_uuid, vdesc); }

    void registerVolume(const VolumeDesc& volDesc)
        { qos_ctrl->registerVolume(volDesc); }


    Error removeVolume(const VolumeDesc& volDesc);

    Error updateQoS(long int const* rate, float const* throttle)
        { return qos_ctrl->updateQoS(rate, throttle); }

    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);

    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);

    bool haveTables();

    bool isShuttingDown() const
    { SCOPEDREAD(shut_down_lock); return shut_down; }

  private:
    Module* parent_mod {nullptr};

    /// QoS Module
    std::unique_ptr<AmQoSCtrl> qos_ctrl;

    std::pair<bool, bool> have_tables;

    bool shut_down { false };
    mutable fds_rwlock shut_down_lock;

    shutdown_cb_type prepareForShutdownCb;

    inline bool haveCacheToken(std::shared_ptr<AmVolume> const& volume) const;

    void respond(AmRequest *amReq, const Error& error);

};

Error
AmProcessor_impl::enqueueRequest(AmRequest* amReq) {
    {
        SCOPEDREAD(shut_down_lock);
        if (shut_down) {
            respond(amReq, ERR_SHUTTING_DOWN);
            return ERR_SHUTTING_DOWN;
        }
        return qos_ctrl->enqueueRequest(amReq);
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

    qos_ctrl.reset(new AmQoSCtrl(qos_threads,
                                 fds::FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET,
                                 provider,
                                 GetLog()));

    auto complete_cb = [this](AmRequest* amReq, Error const& error) mutable -> void {
        this->respond(amReq, error);
    };
    qos_ctrl->init(complete_cb);
}

void
AmProcessor_impl::respond(AmRequest *amReq, const Error& error) {
    if (amReq->cb) {
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
    }

    if (qos_ctrl->drained()) {
        // Close all attached volumes before finishing shutdown
        for (auto const& vol : qos_ctrl->getVolumes()) {
          removeVolume(*vol->voldesc);
        }
        parent_mod->mod_shutdown();
        return true;
    }
    return false;
}

Error
AmProcessor_impl::removeVolume(const VolumeDesc& volDesc) {
    LOGNORMAL << "Removing volume: " << volDesc.name;
    Error err{ERR_OK};

    // Remove the volume from QoS/VolumeTable, this is
    // called to clear any waiting requests with an error and
    // remove the QoS allocations
    return qos_ctrl->removeVolume(volDesc.name, volDesc.volUUID);
}

Error
AmProcessor_impl::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    // If we successfully update the dlt, have the parent do it's init check
    auto e = qos_ctrl->updateDlt(dlt_type, dlt_data, cb);
    if (e.ok() && !have_tables.first) {
        have_tables.first = true;
        parent_mod->mod_enable_service();
    }
    return e;
}

Error
AmProcessor_impl::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    // If we successfully update the dmt, have the parent do it's init check
    auto e = qos_ctrl->updateDmt(dmt_type, dmt_data, cb);
    if (e.ok() && !have_tables.second) {
        have_tables.second = true;
        parent_mod->mod_enable_service();
    }
    return e;
}

bool
AmProcessor_impl::haveTables() {
    if (!have_tables.first) {
        have_tables.first = qos_ctrl->getDLT().ok();
    }
    if (!have_tables.second) {
        have_tables.second = qos_ctrl->getDMT().ok();
    }
    return have_tables.first && have_tables.second;
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
