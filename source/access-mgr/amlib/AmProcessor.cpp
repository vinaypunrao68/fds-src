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
#include "AccessMgr.h"
#include "AmDataProvider.h"
#include "AmRequest.h"
#include "AmVolume.h"
#include "AmVolumeTable.h"
#include "net/SvcMgr.h"
#include "concurrency/RwLock.h"

namespace fds {

/**
 * AM request processing layer. The processor handles state and
 * execution for AM requests.
 */
class AmProcessor_impl : public AmDataProvider
{
    using shutdown_cb_type = std::function<void(void)>;

  public:
    AmProcessor_impl(AccessMgr* parent, CommonModuleProviderIf *modProvider)
        : AmDataProvider(nullptr, nullptr, modProvider),
          parent_mod(parent),
          have_tables(std::make_pair(false, false)),
          prepareForShutdownCb(nullptr)
    { }

    AmProcessor_impl(AmProcessor_impl const&) = delete;
    AmProcessor_impl& operator=(AmProcessor_impl const&) = delete;
    ~AmProcessor_impl() = default;

    void prepareForShutdownMsgRespBindCb(shutdown_cb_type&& cb)
        { std::lock_guard<std::mutex> lk(shut_down_lock); prepareForShutdownCb = cb; }

    void enqueueRequest(AmRequest* amReq);

    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);

    void getVolumes(std::vector<VolumeDesc>& volumes);
    VolumeDesc* getVolume(fds_volid_t const vol_uuid) const;

    bool isShuttingDown() const
    { std::lock_guard<std::mutex> lk(shut_down_lock); return shut_down; }

    /**
     * These are the Processor specific DataProvider routines.
     * Everything else is pass-thru.
     */
    void start() override;
    void stop() override;
    void registerVolume(VolumeDesc const& volDesc) override;
    void removeVolume(VolumeDesc const& volDesc) override;

    void flushVolume(AmRequest* req, std::string const& vol);

  protected:
    void openVolumeCb(AmRequest * amReq, Error const error) override        {respond(amReq, error);}
    void closeVolumeCb(AmRequest * amReq, Error const error) override       {respond(amReq, error);}
    void statVolumeCb(AmRequest * amReq, Error const error) override        {respond(amReq, error);}
    void setVolumeMetadataCb(AmRequest * amReq, Error const error) override {respond(amReq, error);}
    void getVolumeMetadataCb(AmRequest * amReq, Error const error) override {respond(amReq, error);}
    void volumeContentsCb(AmRequest * amReq, Error const error) override    {respond(amReq, error);}
    void startBlobTxCb(AmRequest * amReq, Error const error) override       {respond(amReq, error);}
    void commitBlobTxCb(AmRequest * amReq, Error const error) override      {respond(amReq, error);}
    void abortBlobTxCb(AmRequest * amReq, Error const error) override       {respond(amReq, error);}
    void statBlobCb(AmRequest * amReq, Error const error) override          {respond(amReq, error);}
    void setBlobMetadataCb(AmRequest * amReq, Error const error) override   {respond(amReq, error);}
    void deleteBlobCb(AmRequest * amReq, Error const error) override        {respond(amReq, error);}
    void renameBlobCb(AmRequest * amReq, Error const error) override        {respond(amReq, error);}
    void getBlobCb(AmRequest * amReq, Error const error) override           {respond(amReq, error);}
    void getObjectCb(AmRequest * amReq, Error const error) override         {respond(amReq, error);}
    void putBlobCb(AmRequest * amReq, Error const error) override           {respond(amReq, error);}
    void putBlobOnceCb(AmRequest * amReq, Error const error) override       {respond(amReq, error);}
    void updateCatalogCb(AmRequest * amReq, Error const error) override     {respond(amReq, error);}
    void putObjectCb(AmRequest * amReq, Error const error) override         {respond(amReq, error);}

  private:
    AccessMgr* parent_mod {nullptr};

    fds::fds_rwlock table_lock;
    std::pair<bool, bool> have_tables;
    void haveTables(AmRequest* request);

    mutable std::mutex shut_down_lock;
    std::condition_variable check_done;
    bool shut_down { false };

    shutdown_cb_type prepareForShutdownCb;

    void respond(AmRequest *amReq, const Error& error);
};

void
AmProcessor_impl::enqueueRequest(AmRequest* amReq) {
    haveTables(amReq);
}

void
AmProcessor_impl::start()
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    auto qos_threads = conf.get<int>("qos_threads");
    _next_in_chain.reset(new AmVolumeTable(this,
                                           qos_threads,
                                           GetLog()));
    _next_in_chain->setProvider(getModuleProvider());
    AmDataProvider::start();
}

void
AmProcessor_impl::respond(AmRequest *amReq, const Error& error) {
    if (amReq->cb) {
        bool should_log {false};
        fpi::ErrorCode code {fpi::OK};
        if (!error.ok()) {
            switch (error.GetErrno()) {
                case ERR_DUPLICATE:
                case ERR_HASH_COLLISION:
                case ERR_VOL_DUPLICATE:
                    code = fpi::RESOURCE_ALREADY_EXISTS;
                    break;;
                case ERR_NOT_FOUND:
                    code = fpi::INTERNAL_SERVER_ERROR;
                    should_log = true;
                    break;;
                case ERR_BLOB_NOT_FOUND:
                case ERR_BLOB_OFFSET_INVALID:
                case ERR_CAT_ENTRY_NOT_FOUND:
                case ERR_VOL_NOT_FOUND:
                    code = fpi::MISSING_RESOURCE;
                    break;;
                case ERR_TOKEN_NOT_READY:
                case ERR_NOT_READY:
                case ERR_SVC_REQUEST_FAILED:
                case ERR_SVC_REQUEST_INVOCATION:
                case ERR_VOLUME_ACCESS_DENIED:
                    code = fpi::SERVICE_NOT_READY;
                    should_log = true;
                    break;;
                case ERR_SVC_REQUEST_TIMEOUT:
                    code = fpi::TIMEOUT;
                    should_log = true;
                    break;
                default:
                    code = fpi::BAD_REQUEST;
                    break;;
            }
        }
        if (should_log) {
            LOGERROR << "type:" << amReq->io_type
                     << " blob:" << amReq->getBlobName()
                     << " err:" << error
                     << " code:" << fpi::_ErrorCode_VALUES_TO_NAMES.at(code);
        } else {
            LOGIO << "type:" << amReq->io_type
                  << " blob:" << amReq->getBlobName()
                  << " err:" << error
                  << " code:" << fpi::_ErrorCode_VALUES_TO_NAMES.at(code);
        }
        amReq->cb->call(code);
    }
    delete amReq;

    // In case we're shutting down tell anyone waiting on this conditional
    check_done.notify_all();
}

void AmProcessor_impl::stop() {
    std::unique_lock<std::mutex> lk(shut_down_lock);
    if (!shut_down) {
        LOGNOTIFY << "shutting down AmProcessor";
        shut_down = true;
        AmDataProvider::stop();
    }
    // Wait for all IO to complete and volumes to be closed.
    check_done.wait(lk, [this] () mutable -> bool { return AmDataProvider::done(); });
    if (prepareForShutdownCb) {
        prepareForShutdownCb();
        prepareForShutdownCb = nullptr;
    }
}

void AmProcessor_impl::registerVolume(const VolumeDesc& volDesc) {
    // Alert connectors to the new volume
    parent_mod->volumeAdded(volDesc);
}

void AmProcessor_impl::removeVolume(const VolumeDesc& volDesc) {
    // Alert connectors to the removed volume
    parent_mod->volumeRemoved(volDesc);
    AmDataProvider::removeVolume(volDesc);
}

void AmProcessor_impl::flushVolume(AmRequest* req, std::string const& vol) {
    parent_mod->volumeFlushed(req, vol);
}

VolumeDesc* AmProcessor_impl::getVolume(fds_volid_t const vol_uuid) const {
    auto volTable = dynamic_cast<AmVolumeTable*>(_next_in_chain.get());
    if (nullptr == volTable) {
        return nullptr;
    } else {
        auto vol = volTable->getVolume(vol_uuid);
        if (nullptr == vol) return nullptr;
        else return vol->voldesc;
    }
}

Error
AmProcessor_impl::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    {
        std::lock_guard<std::mutex> lg(shut_down_lock);
        if (shut_down) return ERR_OK;
    }
    WriteGuard g(table_lock);
    // If we successfully update the dlt, have the parent do it's init check
    auto err = MODULEPROVIDER()->getSvcMgr()->updateDlt(dlt_type, dlt_data, cb);
    if (err.ok() || ERR_DUPLICATE == err) {
        have_tables.first = true;
        err = ERR_OK;
    }
    return err;
}

Error
AmProcessor_impl::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    {
        std::lock_guard<std::mutex> lg(shut_down_lock);
        if (shut_down) return ERR_OK;
    }
    WriteGuard g(table_lock);
    // If we successfully update the dmt, have the parent do it's init check
    auto err = MODULEPROVIDER()->getSvcMgr()->updateDmt(dmt_type, dmt_data, cb);
    if (err.ok() || ERR_DUPLICATE == err) {
        have_tables.second = true;
        err = ERR_OK;
    }
    return err;
}

void
AmProcessor_impl::haveTables(AmRequest* request) {
    {
        std::lock_guard<std::mutex> lg(shut_down_lock);
        if (shut_down) {
            respond(request, ERR_NOT_READY);
            return;
        }
    }
    table_lock.cond_write_lock();
    auto& have_dlt = have_tables.first;
    auto& have_dmt = have_tables.second;
    if (!have_dlt || !have_dmt) {
        table_lock.upgrade();
        auto get_dlt = MODULEPROVIDER()->getSvcMgr()->getDLT();
        auto get_dmt = MODULEPROVIDER()->getSvcMgr()->getDMT();
        have_dlt = (ERR_OK == get_dlt || ERR_DUPLICATE == get_dlt);
        have_dmt = (ERR_OK == get_dmt || ERR_DUPLICATE == get_dmt);
        table_lock.write_unlock();
    } else {
        table_lock.cond_write_unlock();
    }
    if (have_dlt && have_dmt) {
        unknownTypeResume(request);
    } else {
        respond(request, ERR_NOT_READY);
    }
}

void
AmProcessor_impl::getVolumes(std::vector<VolumeDesc>& volumes) {
    std::lock_guard<std::mutex> lg(shut_down_lock);
    volumes.clear();
    if (shut_down) return;
    Error err(ERR_OK);
    fpi::GetAllVolumeDescriptors list;
    err = MODULEPROVIDER()->getSvcMgr()->getAllVolumeDescriptors(list);
    if (err.ok()) {
        for (auto const& volume : list.volumeList) {
            if (fpi::Active == volume.vol_desc.state) {
                volumes.emplace_back(volume.vol_desc);
            }
        }
    }
}

/**
 * Pimpl forwarding methods. Should just call the underlying implementaion
 */
AmProcessor::AmProcessor(AccessMgr* parent, CommonModuleProviderIf *modProvider)
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

void AmProcessor::stop()
{ return _impl->stop(); }

void AmProcessor::enqueueRequest(AmRequest* amReq)
{ return _impl->enqueueRequest(amReq); }

void AmProcessor::getVolumes(std::vector<VolumeDesc>& volumes)
{ return _impl->getVolumes(volumes); }

bool AmProcessor::isShuttingDown() const
{ return _impl->isShuttingDown(); }

Error AmProcessor::modifyVolumePolicy(const VolumeDesc& vdesc)
{
    auto err = _impl->modifyVolumePolicy(vdesc);
    if (ERR_VOL_NOT_FOUND == err) {
        err = ERR_OK;
    }
    return err;
}
void AmProcessor::addToVolumeGroup(const FDS_ProtocolInterface::AddToVolumeGroupCtrlMsgPtr &addMsg,
                                   const AddToVolumeGroupCb &cb)
{
    _impl->addToVolumeGroup(addMsg, cb);
}

void AmProcessor::registerVolume(const VolumeDesc& volDesc)
{ return _impl->registerVolume(volDesc); }

void AmProcessor::removeVolume(const VolumeDesc& volDesc)
{ return _impl->removeVolume(volDesc); }

void AmProcessor::flushVolume(AmRequest* req, std::string const& vol)
{ return _impl->flushVolume(req, vol); }

VolumeDesc* AmProcessor::getVolume(fds_volid_t const vol_uuid) const
{ return _impl->getVolume(vol_uuid); }

Error AmProcessor::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb)
{ return _impl->updateDlt(dlt_type, dlt_data, cb); }

Error AmProcessor::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb)
{ return _impl->updateDmt(dmt_type, dmt_data, cb); }

Error AmProcessor::updateQoS(long int const* rate, float const* throttle)
{ return _impl->updateQoS(rate, throttle); }

}  // namespace fds
