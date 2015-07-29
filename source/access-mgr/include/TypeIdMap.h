/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_TYPEIDMAP_H_
#define SOURCE_ACCESS_MGR_INCLUDE_TYPEIDMAP_H_

#include "fdsp/svc_types_types.h"
#include "fdsp/dm_api_types.h"
#include "fdsp/sm_api_types.h"
#include "fdsp_utils.h"

namespace fds
{

// Unfortunately, the message ids for our APIs are not bound in anyway to the
// message structures themselves, but are a manipulation of the symbol name.
// These functions help make the code more generic below.
// =============
template<typename Msg> fpi::FDSPMsgTypeId constexpr message_type_id(Msg const& msg);

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::AbortBlobTxMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::AbortBlobTxMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::CloseVolumeMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::CloseVolumeMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::CommitBlobTxMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::CommitBlobTxMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::DeleteBlobMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::DeleteBlobMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::GetBlobMetaDataMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::GetBlobMetaDataMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::GetBucketMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::GetBucketMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::GetVolumeMetadataMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::GetVolumeMetadataMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::OpenVolumeMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::OpenVolumeMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::QueryCatalogMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::QueryCatalogMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::SetBlobMetaDataMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::SetBlobMetaDataMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::SetVolumeMetadataMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::SetVolumeMetadataMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::StartBlobTxMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::StartBlobTxMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::StatVolumeMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::StatVolumeMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::RenameBlobMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::RenameBlobMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::UpdateCatalogMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::UpdateCatalogOnceMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::UpdateCatalogOnceMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::PutObjectMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::PutObjectMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::GetObjectMsg const& msg)
{ return FDSP_MSG_TYPEID(fpi::GetObjectMsg); }
// =============

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_TYPEIDMAP_H_
