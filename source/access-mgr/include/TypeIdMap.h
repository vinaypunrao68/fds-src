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

// Unfortunately, the message ids for our APIs are not bound in anyways to the
// message structures themselves, but are a manipulation of the symbol name.
// These functions help make the code more generic below.
// =============
template<typename Msg> fpi::FDSPMsgTypeId constexpr message_type_id(Msg const& msg);

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::AbortBlobTxMsg const& msg)
{ return FDSP_MSG_TYPEID(AbortBlobTxMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::CloseVolumeMsg const& msg)
{ return FDSP_MSG_TYPEID(CloseVolumeMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::CommitBlobTxMsg const& msg)
{ return FDSP_MSG_TYPEID(CommitBlobTxMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::DeleteBlobMsg const& msg)
{ return FDSP_MSG_TYPEID(DeleteBlobMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::GetBlobMetaDataMsg const& msg)
{ return FDSP_MSG_TYPEID(GetBlobMetaDataMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::GetBucketMsg const& msg)
{ return FDSP_MSG_TYPEID(GetBucketMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::GetVolumeMetadataMsg const& msg)
{ return FDSP_MSG_TYPEID(GetVolumeMetadataMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::OpenVolumeMsg const& msg)
{ return FDSP_MSG_TYPEID(OpenVolumeMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::QueryCatalogMsg const& msg)
{ return FDSP_MSG_TYPEID(QueryCatalogMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::SetBlobMetaDataMsg const& msg)
{ return FDSP_MSG_TYPEID(SetBlobMetaDataMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::SetVolumeMetadataMsg const& msg)
{ return FDSP_MSG_TYPEID(SetVolumeMetadataMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::StartBlobTxMsg const& msg)
{ return FDSP_MSG_TYPEID(StartBlobTxMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::StatVolumeMsg const& msg)
{ return FDSP_MSG_TYPEID(StatVolumeMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::UpdateCatalogMsg const& msg)
{ return FDSP_MSG_TYPEID(UpdateCatalogMsg); }

template<> fpi::FDSPMsgTypeId constexpr message_type_id(fpi::UpdateCatalogOnceMsg const& msg)
{ return FDSP_MSG_TYPEID(UpdateCatalogOnceMsg); }
// =============

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_TYPEIDMAP_H_
