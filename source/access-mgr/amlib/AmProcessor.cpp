/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */

#include <AmProcessor.h>

#include <algorithm>
#include <deque>
#include <string>

#include "fdsp/common_types.h"
#include "fds_process.h"
#include "fds_table.h"
#include <fiu-control.h>
#include <util/fiu_util.h>
#include "AmDataProvider.h"
#include "AmRequest.h"
#include "AmVolumeTable.h"

namespace fds {

/**
 * AM request processing layer. The processor handles state and
 * execution for AM requests.
 */
class AmProcessor_impl : public AmDataProvider
{
    using shutdown_cb_type = std::function<void(void)>;
    CommonModuleProviderIf* provider;

  public:
    AmProcessor_impl(Module* parent, CommonModuleProviderIf *modProvider)
        : AmDataProvider(nullptr, nullptr),
          provider(modProvider),
          parent_mod(parent),
          have_tables(std::make_pair(false, false)),
          prepareForShutdownCb(nullptr)
    { }

    AmProcessor_impl(AmProcessor_impl const&) = delete;
    AmProcessor_impl& operator=(AmProcessor_impl const&) = delete;
    ~AmProcessor_impl() = default;

    void prepareForShutdownMsgRespBindCb(shutdown_cb_type&& cb)
        { prepareForShutdownCb = cb; }

    void prepareForShutdownMsgRespCallCb();


    void enqueueRequest(AmRequest* amReq);

    bool haveTables();

    bool isShuttingDown() const
    { SCOPEDREAD(shut_down_lock); return shut_down; }

    /**
     * These are the Processor specific DataProvider routines.
     * Everything else is pass-thru.
     */
    void start() override;
    void stop() override;
    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) override;
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) override;

  protected:
    /**
     * These are only here because AmDataApi is not a data provider
     */
    void openVolumeCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void closeVolumeCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void statVolumeCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void setVolumeMetadataCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void getVolumeMetadataCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void volumeContentsCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void startBlobTxCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void commitBlobTxCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void abortBlobTxCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void statBlobCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void setBlobMetadataCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void deleteBlobCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void renameBlobCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void getBlobCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void putBlobCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

    void putBlobOnceCb(AmRequest * amReq, Error const error) override
    { respond(amReq, error); }

  private:
    Module* parent_mod {nullptr};

    std::pair<bool, bool> have_tables;

    bool shut_down { false };
    mutable fds_rwlock shut_down_lock;

    shutdown_cb_type prepareForShutdownCb;

    void respond(AmRequest *amReq, const Error& error);
};

void
AmProcessor_impl::enqueueRequest(AmRequest* amReq) {
    {
        SCOPEDREAD(shut_down_lock);
        if (shut_down) {
            respond(amReq, ERR_SHUTTING_DOWN);
            return;
        }
    }
    AmDataProvider::unknownTypeResume(amReq);
}

void
AmProcessor_impl::start()
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    if (conf.get<fds_bool_t>("testing.uturn_processor_all")) {
        fiu_enable("am.uturn.processor.*", 1, NULL, 0);
    }
    auto qos_threads = conf.get<int>("qos_threads");
    _next_in_chain.reset(new AmVolumeTable(this,
                                           qos_threads,
                                           provider,
                                           GetLog()));
    AmDataProvider::start();
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
                case ERR_BLOB_OFFSET_INVALID:
                case ERR_CAT_ENTRY_NOT_FOUND:
                case ERR_VOL_NOT_FOUND:
                    code = fpi::MISSING_RESOURCE;
                    break;;
                case ERR_NOT_READY:
                case ERR_VOLUME_ACCESS_DENIED:
                    code = fpi::SERVICE_NOT_READY;
                    break;;
                default:
                    code = fpi::BAD_REQUEST;
                    break;;
            }
        }
        LOGIO << amReq->io_type
              << " on: [" << std::hex << amReq->getBlobName()
              << "] had result: [" << error
              << "] API code: [" << fpi::_ErrorCode_VALUES_TO_NAMES.at(code) << "]";
        amReq->cb->call(code);
    }
    delete amReq;

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

void AmProcessor_impl::stop() {
    SCOPEDWRITE(shut_down_lock);

    if (!shut_down) {
        LOGNOTIFY << "AmProcessor received a stop request.";
        shut_down = true;
        AmDataProvider::stop();
    }

    if (AmDataProvider::done()) {
        parent_mod->mod_shutdown();
    }
}

Error
AmProcessor_impl::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    // If we successfully update the dlt, have the parent do it's init check
    auto e = AmDataProvider::updateDlt(dlt_type, dlt_data, cb);
    if (e.ok() && !have_tables.first) {
        have_tables.first = true;
        parent_mod->mod_enable_service();
    }
    return e;
}

Error
AmProcessor_impl::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    // If we successfully update the dmt, have the parent do it's init check
    auto e = AmDataProvider::updateDmt(dmt_type, dmt_data, cb);
    if (e.ok() && !have_tables.second) {
        have_tables.second = true;
        parent_mod->mod_enable_service();
    }
    return e;
}

bool
AmProcessor_impl::haveTables() {
    if (!have_tables.first) {
        have_tables.first = AmDataProvider::getDLT().ok();
    }
    if (!have_tables.second) {
        have_tables.second = AmDataProvider::getDMT().ok();
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

void AmProcessor::stop()
{ return _impl->stop(); }

void AmProcessor::enqueueRequest(AmRequest* amReq)
{ return _impl->enqueueRequest(amReq); }

bool AmProcessor::haveTables()
{ return _impl->haveTables(); }

bool AmProcessor::isShuttingDown() const
{ return _impl->isShuttingDown(); }

Error AmProcessor::modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc)
{ return _impl->modifyVolumePolicy(vol_uuid, vdesc); }

void AmProcessor::registerVolume(const VolumeDesc& volDesc)
{ return _impl->registerVolume(volDesc); }

void AmProcessor::removeVolume(const VolumeDesc& volDesc)
{ return _impl->removeVolume(volDesc); }

Error AmProcessor::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb)
{ return _impl->updateDlt(dlt_type, dlt_data, cb); }

Error AmProcessor::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb)
{ return _impl->updateDmt(dmt_type, dmt_data, cb); }

Error AmProcessor::updateQoS(long int const* rate, float const* throttle)
{ return _impl->updateQoS(rate, throttle); }

}  // namespace fds
