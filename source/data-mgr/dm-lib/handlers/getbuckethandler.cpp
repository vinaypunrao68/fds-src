/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <stack>
#include <string>
#include <tuple>
#include <list>
#include <algorithm>

#include <pcrecpp.h>

#include <DataMgr.h>
#include <dmhandler.h>
#include <DMSvcHandler.h>

using std::get;
using std::string;
using std::stack;
using std::tuple;

using FDS_ProtocolInterface::PatternSemantics;

namespace fds {
namespace dm {

GetBucketHandler::GetBucketHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::GetBucketMsg, handleRequest);
    }
}

void GetBucketHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                     boost::shared_ptr<fpi::GetBucketMsg>& message) {
    fds_volid_t volId(message->volume_id);
    LOGDEBUG << "volume: " << volId;

    Error err(ERR_OK);
    if (!dataManager.amIPrimaryGroup(volId)) {
    	err = ERR_DM_NOT_PRIMARY;
    }
    if (err.OK()) {
    	err = dataManager.validateVolumeIsActive(volId);
    }
    if (!err.OK())
    {
        auto dummyResponse = boost::make_shared<fpi::GetBucketRspMsg>();
        handleResponse(asyncHdr, dummyResponse, err, nullptr);
        return;
    }

    // setup the request
    auto dmRequest = new DmIoGetBucket(message);

    // setup callback
    dmRequest->cb = BIND_MSG_CALLBACK(GetBucketHandler::handleResponse, asyncHdr, dmRequest->response);

    addToQueue(dmRequest);
}

void GetBucketHandler::handleQueueItem(DmRequest *dmRequest) {
    QueueHelper helper(dataManager, dmRequest);  // this will call the callback
    DmIoGetBucket *request = static_cast<DmIoGetBucket*>(dmRequest);

    fpi::BlobDescriptorListType & blobVec = request->response->blob_descr_list;
    // do processing and set the error
    helper.err = dataManager.timeVolCat_->queryIface()->listBlobs(dmRequest->volId, &blobVec);

    // match pattern if specified
    auto& patternString = request->message->pattern;
    if (!patternString.empty()) {

        string semanticPattern;

        // Compile-time error for missing a value.
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch-enum"
        switch (request->message->patternSemantics)
        {
        case PatternSemantics::PCRE:
            semanticPattern = patternString;
            break;
        case PatternSemantics::DOS:
            semanticPattern = _dosGlobToPcre(patternString);
            break;
        case PatternSemantics::UNIX:
            semanticPattern = _bashGlobToPcre(patternString);
            break;
        case PatternSemantics::LITERAL:
            semanticPattern = pcrecpp::RE::QuoteMeta(patternString);
            break;
        }
#pragma GCC diagnostic pop

        pcrecpp::RE pattern(semanticPattern, pcrecpp::UTF8());
        if (!pattern.error().empty()) {
            LOGWARN << "Error initializing pattern: " << quoteString(patternString)
                    << " " << pattern.error();
            helper.err = ERR_DM_INVALID_REGEX;
            return;
        }

        auto iterEnd = std::remove_if(blobVec.begin(), blobVec.end(),
                [&pattern](const fpi::BlobDescriptor & blobDescr) {
                    return !pattern.PartialMatch(blobDescr.name);
                });
        blobVec.erase(iterEnd, blobVec.end());
    }

    // sort if required
    if (request->message->orderBy) {
        const fpi::BlobListOrder orderColumn = request->message->orderBy;
        const bool descending = request->message->descending;
        std::sort(blobVec.begin(), blobVec.end(),
                [orderColumn, descending](const fpi::BlobDescriptor & lhs,
                                          const fpi::BlobDescriptor & rhs) {
                    bool ret = fpi::BLOBSIZE == orderColumn ?  lhs.byteCount < rhs.byteCount :
                            lhs.name < rhs.name;
                    return descending ? !ret : ret;
                });
    }

    // based on user's request process results
    if (request->message->startPos) {
        if (static_cast<fds_uint64_t>(request->message->startPos) < blobVec.size()) {
            blobVec.erase(blobVec.begin(), blobVec.begin() + request->message->startPos);
        } else {
            blobVec.clear();
        }
    }
    if (!blobVec.empty() && blobVec.size() > static_cast<fds_uint32_t>(request->message->count)) {
        blobVec.erase(blobVec.begin() + request->message->count, blobVec.end());
    }

    LOGDEBUG << " volid: " << dmRequest->volId
             << " numblobs: " << request->response->blob_descr_list.size();
}

void GetBucketHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      boost::shared_ptr<fpi::GetBucketRspMsg>& message,
                                      const Error &e, DmRequest *dmRequest) {
    LOGDEBUG << " volid: " << (dmRequest ? dmRequest->volId : invalid_vol_id) << " err: " << e;
    asyncHdr->msg_code = static_cast<int32_t>(e.GetErrno());
    DM_SEND_ASYNC_RESP(asyncHdr, fpi::GetBucketRspMsgTypeId, message);
    delete dmRequest;
}

string GetBucketHandler::_bashCurlyToPcre(string const& glob, size_t& characterIndex)
{
    string retval;

    auto start = characterIndex;

    bool escaping = false;
    while (characterIndex < glob.length())
    {
        char character = glob[characterIndex++];

        if (escaping)
        {
            retval.append(pcrecpp::RE::QuoteMeta(string(1, character)));
            escaping = false;
        }
        else
        {
            switch (character)
            {
            case '\\':
                escaping = true;
                break;
            case ',':
                retval.push_back('|');
                break;
            case '{':
                retval.append(_bashCurlyToPcre(glob, characterIndex));
                break;
            case '}':
                if (retval.empty())
                {
                    characterIndex = start;
                    return pcrecpp::RE::QuoteMeta("{");
                }
                else
                {
                    return "(?:" + retval + ")";
                }
            default:
                retval.append(pcrecpp::RE::QuoteMeta(string(1, character)));
                break;
            }
        }
    }

    // Ran out of characters before finding a close.
    characterIndex = start;
    return pcrecpp::RE::QuoteMeta("{");
}

string GetBucketHandler::_bashGlobToPcre(string const& glob)
{
    string retval;

    bool escaping = false;
    size_t characterIndex = 0;
    while (characterIndex < glob.length())
    {
        char character = glob[characterIndex++];

        if (escaping)
        {
            retval.append(pcrecpp::RE::QuoteMeta(string(1, character)));
            escaping = false;
        }
        else
        {
            switch (character)
            {
            case '\\':
                escaping = true;
                break;
            case '?':
                retval.push_back('.');
                break;
            case '*':
                retval.append(".*");
                break;
            case '[':
                retval.append(_bashSquareToPcre(glob, characterIndex));
                break;
            case '{':
                retval.append(_bashCurlyToPcre(glob, characterIndex));
                break;
            default:
                retval.append(pcrecpp::RE::QuoteMeta(string(1, character)));
            }
        }
    }

    return retval;
}

string GetBucketHandler::_bashSquareToPcre(string const& glob, size_t& characterIndex)
{
    string retval;

    auto start = characterIndex;

    bool inverse = false;
    while (characterIndex < glob.length())
    {
        char character = glob[characterIndex++];

        switch (character)
        {
        case '-':
            // Range syntax is the same, so we can just pass this on.
            retval.push_back(character);
            break;
        case '!':
            if (retval.empty())
            {
                inverse = true;
            }
            else
            {
                retval.append(pcrecpp::RE::QuoteMeta(string(1, character)));
            }
            break;
        case ']':
            if (retval.empty())
            {
                // Can't have an empty group, so these can come in unescaped. If we don't find
                // another closing brace, we take care of it when we run out of characters.
                retval.append(pcrecpp::RE::QuoteMeta(string(1, character)));
            }
            else
            {
                // Normal exit.
                return string("[") + (inverse ? "^" : "") + retval + "]";
            }
            break;
        default:
            retval.append(pcrecpp::RE::QuoteMeta(string(1, character)));
            break;
        }
    }

    // Ran out of characters before finding a close.
    if (retval.empty())
    {
        // No close, interpret as regular characters.
        if (inverse)
        {
            characterIndex = start + 2;
            return pcrecpp::RE::QuoteMeta("[!");
        }
        else
        {
            characterIndex = start + 1;
            return pcrecpp::RE::QuoteMeta("[");
        }
    }
    else
    {
        // Close in first character, inversion is a regular character and only content.
        if (retval[0] == ']')
        {
            characterIndex = start + 3;
            return string("[") + (inverse ? "!" : "") + "]";
        }
        else
        {
            // Without close, it's not a character group, so inversion doesn't mean anything.
            characterIndex = start + 1;
            return "[";
        }
    }
}

string GetBucketHandler::_dosGlobToPcre(string const& glob)
{
    string retval;
    for (char character : glob)
    {
        switch (character)
        {
        case '?':
            retval.push_back('.');
            break;
        case '*':
            retval.append(".*");
            break;
        default:
            retval.push_back(character);
        }
    }
    return retval;
}
}  // namespace dm
}  // namespace fds
