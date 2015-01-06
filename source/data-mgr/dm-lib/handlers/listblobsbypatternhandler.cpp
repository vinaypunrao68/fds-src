/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in BaseAsyncSvcHandler.h
#include <net/BaseAsyncSvcHandler.h>
#include <PerfTrace.h>

#include <boost/algorithm/string/replace.hpp>
#include <pcrecpp.h>

#include <map>
#include <string>

using fpi::AsyncHdr;
using fpi::BlobInfoListType;
using fpi::FDSP_MetaDataList;
using fpi::ListBlobsByPatternMsg;
using fpi::ListBlobsByPatternMsgTypeId;
using fpi::ListBlobsByPatternRspMsg;
using fpi::ListBlobsByPatternRspMsgTypeId;

using boost::replace_all;

using pcrecpp::RE;
using pcrecpp::UTF8;

using std::map;
using std::string;

using ThriftBlobDescriptor = fpi::BlobDescriptor;

namespace fds {
namespace dm {

ListBlobsByPatternHandler::ListBlobsByPatternHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(ListBlobsByPatternMsg, handleRequest);
    }
}

void ListBlobsByPatternHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<ListBlobsByPatternMsg>& message) {
    LOGDEBUG << logString(*asyncHdr) << logString(*message);

    auto dmReq = new DmIoListBlobsByPattern(message);
    dmReq->cb = BIND_MSG_CALLBACK(ListBlobsByPatternHandler::handleResponse, asyncHdr, message);

    PerfTracer::tracePointBegin(dmReq->opReqLatencyCtx);

    addToQueue(dmReq);
}

void ListBlobsByPatternHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dmRequest);
    auto typedRequest = static_cast<DmIoListBlobsByPattern*>(dmRequest);
    auto& response = *typedRequest->response;

    // TODO(DAC): Add error reporting to response message.
    RE pattern(typedRequest->message->pattern, UTF8());
    if (!pattern.error().empty()) {
        LOGWARN << "Error initializing pattern: " << quoteString(typedRequest->message->pattern)
                << " " << pattern.error();
        return;
    }

    // FIXME(DAC): This must be done within the context of a transaction--blobs may be added or
    //             deleted between retrieving the list and pulling metadata.

    // TODO(DAC): Support paged queries (typedRequest->message->maxKeys, startPos). No support in
    //            underlying APIs yet. Will need to do additional work as filtered list offsets
    //            may (usually won't) match up with unfiltered offsets.

    BlobInfoListType allBlobs;
    helper.err = dataMgr->timeVolCat_->queryIface()->listBlobs(dmRequest->volId, &allBlobs);
    if (!helper.err.ok()) {
        return;
    }

    auto& blobDescriptors = response.blobDescriptors;
    for (auto const& blobInfo : allBlobs) {
        if (pattern.PartialMatch(blobInfo.blob_name)) {
            // TODO(DAC): Blob metadata shouldn't require all this copying and temporaries.
            blob_version_t blob_version;
            fds_uint64_t blob_size;
            FDSP_MetaDataList meta_list;

            if (dataMgr->timeVolCat_->queryIface()->getBlobMeta(typedRequest->volId,
                                                                blobInfo.blob_name,
                                                                &blob_version,
                                                                &blob_size,
                                                                &meta_list)
                    .ok()) {
                blobDescriptors.emplace_back();
                auto& descriptor = blobDescriptors.back();
                descriptor.name = blobInfo.blob_name;
                descriptor.byteCount = blob_size;
                descriptor.metadata = map<string, string>();
                for (auto const& meta_pair : meta_list) {
                    descriptor.metadata[meta_pair.key] = meta_pair.value;
                }
            }
        }
    }
}

void ListBlobsByPatternHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                               boost::shared_ptr<ListBlobsByPatternMsg>& message,
                                               Error const& e, dmCatReq* dmRequest) {
    auto typedRequest = static_cast<DmIoListBlobsByPattern*>(dmRequest);

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, ListBlobsByPatternRspMsgTypeId, *typedRequest->response);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
