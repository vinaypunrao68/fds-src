/*
 * Copyright 2015-2016 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMDATAPROVIDER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMDATAPROVIDER_H_

#include <stdexcept>
#include <string>
#include <memory>

#include "fds_error.h"
#include "fds_table.h"
#include "fds_volume.h"

namespace fds {

struct AmRequest;

/**
 * AM's DataProvider API
 * All service layers of AM from QoS to Dispatcher implement this interface
 */
struct AmDataProvider {
    explicit AmDataProvider(AmDataProvider* _prev, AmDataProvider* _next) :
        _next_in_chain(_next),
        _prev_in_chain(_prev)
    { }

    virtual ~AmDataProvider() = default;

    virtual void start()
    { forward_request(&AmDataProvider::start); }

    virtual void stop()
    { forward_request(&AmDataProvider::stop); }

    virtual bool done()
    {
        if (_next_in_chain) {
            return _next_in_chain->done();
        }
        return true;
    }

    virtual void getVolumes(std::vector<VolumeDesc>& volumes)
    {
        if (_next_in_chain) {
            return _next_in_chain->getVolumes(volumes);
        }
        volumes.clear();
    }

    virtual void registerVolume(const VolumeDesc& volDesc)
    {
        if (_next_in_chain) {
            return _next_in_chain->registerVolume(volDesc);
        }
    }

    virtual void removeVolume(const VolumeDesc& volDesc)
    {
        if (_next_in_chain) {
            return _next_in_chain->removeVolume(volDesc);
        }
    }

    virtual Error modifyVolumePolicy(VolumeDesc const& volDesc)
    {
        if (_next_in_chain) {
            return _next_in_chain->modifyVolumePolicy(volDesc);
        }
        return ERR_OK;
    }

    virtual void lookupVolume(std::string const volume_name)
    { return forward_request(&AmDataProvider::lookupVolume, volume_name); }

    virtual Error updateQoS(int64_t const* rate, float const* throttle)
    { return forward_request(&AmDataProvider::updateQoS, rate, throttle); }

    virtual void openVolume(AmRequest * amReq)
    { return forward_request(&AmDataProvider::openVolume, amReq); }

    virtual void closeVolume(AmRequest * amReq)
    { return forward_request(&AmDataProvider::closeVolume, amReq); }

    virtual void statVolume(AmRequest * amReq)
    { return forward_request(&AmDataProvider::statVolume, amReq); }

    virtual void setVolumeMetadata(AmRequest * amReq)
    { return forward_request(&AmDataProvider::setVolumeMetadata, amReq); }

    virtual void getVolumeMetadata(AmRequest * amReq)
    { return forward_request(&AmDataProvider::getVolumeMetadata, amReq); }

    virtual void volumeContents(AmRequest * amReq)
    { return forward_request(&AmDataProvider::volumeContents, amReq); }

    virtual void startBlobTx(AmRequest * amReq)
    { return forward_request(&AmDataProvider::startBlobTx, amReq); }

    virtual void commitBlobTx(AmRequest * amReq)
    { return forward_request(&AmDataProvider::commitBlobTx, amReq); }

    virtual void abortBlobTx(AmRequest * amReq)
    { return forward_request(&AmDataProvider::abortBlobTx, amReq); }

    virtual void statBlob(AmRequest * amReq)
    { return forward_request(&AmDataProvider::statBlob, amReq); }

    virtual void setBlobMetadata(AmRequest * amReq)
    { return forward_request(&AmDataProvider::setBlobMetadata, amReq); }

    virtual void deleteBlob(AmRequest * amReq)
    { return forward_request(&AmDataProvider::deleteBlob, amReq); }

    virtual void renameBlob(AmRequest * amReq)
    { return forward_request(&AmDataProvider::renameBlob, amReq); }

    virtual void getBlob(AmRequest * amReq)
    { return forward_request(&AmDataProvider::getBlob, amReq); }

    virtual void getOffsets(AmRequest * amReq)
    { return forward_request(&AmDataProvider::getOffsets, amReq); }

    virtual void getObject(AmRequest * amReq)
    { return forward_request(&AmDataProvider::getObject, amReq); }

    virtual void putBlob(AmRequest * amReq)
    { return forward_request(&AmDataProvider::putBlob, amReq); }

    virtual void putBlobOnce(AmRequest * amReq)
    { return forward_request(&AmDataProvider::putBlobOnce, amReq); }

    // If you don't know the type of request, use this to get runtime routing
    void unknownTypeResume(AmRequest * amReq);

    // If you don't know the type of request, use this to get runtime routing
    void unknownTypeCb(AmRequest * amReq, Error const error);

    virtual Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
        if (_next_in_chain) {
            return _next_in_chain->updateDlt(dlt_type, dlt_data, cb);
        }
        throw std::runtime_error("Unimplemented DataProvider routine.");
    }

    virtual Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
        if (_next_in_chain) {
            return _next_in_chain->updateDmt(dmt_type, dmt_data, cb);
        }
        throw std::runtime_error("Unimplemented DataProvider routine.");
    }

    virtual Error getDMT()
    { return forward_request(&AmDataProvider::getDMT); }

    virtual Error getDLT()
    { return forward_request(&AmDataProvider::getDLT); }

 protected:
    virtual void lookupVolumeCb(VolumeDesc const vol_desc, Error const error)
    { return forward_response(&AmDataProvider::lookupVolumeCb, vol_desc, error); }

    virtual void openVolumeCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::openVolumeCb, amReq, error); }

    virtual void closeVolumeCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::closeVolumeCb, amReq, error); }

    virtual void statVolumeCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::statVolumeCb, amReq, error); }

    virtual void setVolumeMetadataCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::setVolumeMetadataCb, amReq, error); }

    virtual void getVolumeMetadataCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::getVolumeMetadataCb, amReq, error); }

    virtual void volumeContentsCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::volumeContentsCb, amReq, error); }

    virtual void startBlobTxCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::startBlobTxCb, amReq, error); }

    virtual void commitBlobTxCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::commitBlobTxCb, amReq, error); }

    virtual void abortBlobTxCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::abortBlobTxCb, amReq, error); }

    virtual void statBlobCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::statBlobCb, amReq, error); }

    virtual void setBlobMetadataCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::setBlobMetadataCb, amReq, error); }

    virtual void deleteBlobCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::deleteBlobCb, amReq, error); }

    virtual void renameBlobCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::renameBlobCb, amReq, error); }

    virtual void getBlobCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::getBlobCb, amReq, error); }

    virtual void getOffsetsCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::getOffsetsCb, amReq, error); }

    virtual void getObjectCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::getObjectCb, amReq, error); }

    virtual void putBlobCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::putBlobCb, amReq, error); }

    virtual void putBlobOnceCb(AmRequest * amReq, Error const error)
    { return forward_response(&AmDataProvider::putBlobOnceCb, amReq, error); }

    AmDataProvider* getNextInChain() const
    { return _next_in_chain.get(); }

    std::unique_ptr<AmDataProvider> _next_in_chain {nullptr};
    AmDataProvider* const _prev_in_chain {nullptr};

 protected:
    template<typename Ret, typename ... Args>
    constexpr Ret forward_request(Ret (AmDataProvider::*func)(Args...), Args... args) {
        if (_next_in_chain) {
            return ((_next_in_chain.get())->*(func))(args...);
        }
        fds_panic("here");
        throw std::runtime_error("Unimplemented DataProvider routine.");
    }

    template<typename ... Args>
    constexpr void forward_response(void (AmDataProvider::*func)(Args...), Args... args) {
        if (_prev_in_chain) {
            return ((_prev_in_chain)->*(func))(args...);
        }
        fds_panic("here");
        throw std::runtime_error("Unimplemented DataProvider routine.");
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMDATAPROVIDER_H_
