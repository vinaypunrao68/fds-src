/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "connector/BlockTask.h"

#include <boost/make_shared.hpp>

namespace fds
{

void
BlockTask::handleReadResponse(std::vector<boost::shared_ptr<std::string>>& buffers,
                                      fds_uint32_t len) {
    static boost::shared_ptr<std::string> const empty_buffer =
        boost::make_shared<std::string>(maxObjectSizeInBytes, '\0');
    fds_assert(operation == READ);

    // acquire the buffers
    bufVec.swap(buffers);

    // Fill in any missing wholes with zero data, this is a special *block*
    // semantic for NULL objects.
    for (auto& buf: bufVec) {
        if (!buf) {
            buf = empty_buffer;
            len += maxObjectSizeInBytes;
        }
    }

    // return zeros for uninitialized objects, again a special *block*
    // semantic to PAD the read to the required length.
    uint32_t iOff = offset % maxObjectSizeInBytes;
    if (len < (length + iOff)) {
        for (ssize_t zero_data = (length + iOff) - len; 0 < zero_data; zero_data -= maxObjectSizeInBytes) {
            bufVec.push_back(empty_buffer);
        }
    }

    // Trim the data as needed from the front...
    auto firstObjLen = std::min(length, maxObjectSizeInBytes - iOff);
    if (maxObjectSizeInBytes != firstObjLen) {
        bufVec.front() = boost::make_shared<std::string>(bufVec.front()->data() + iOff, firstObjLen);
    }

    // ...and the back
    if (length > firstObjLen) {
        auto padding = (2 < bufVec.size()) ? (bufVec.size() - 2) * maxObjectSizeInBytes : 0;
        auto lastObjLen = length - firstObjLen - padding;
        if (0 < lastObjLen && maxObjectSizeInBytes != lastObjLen) {
            bufVec.back() = boost::make_shared<std::string>(bufVec.back()->data(), lastObjLen);
        }
    }
}

std::pair<Error, boost::shared_ptr<std::string>>
BlockTask::handleRMWResponse(boost::shared_ptr<std::string> const& retBuf,
                                 uint32_t len,
                                 uint32_t seqId,
                                 const Error& err) {
    fds_assert(operation == WRITE);
    if (!err.ok() && (err != ERR_BLOB_OFFSET_INVALID) &&
                     (err != ERR_BLOB_NOT_FOUND)) {
        opError = err;
        return std::make_pair(err, boost::shared_ptr<std::string>());
    }

    uint32_t iOff = (seqId == 0) ? offset % maxObjectSizeInBytes : 0;
    auto& writeBytes = bufVec[seqId];
    boost::shared_ptr<std::string> fauxBytes;
    if ((err == ERR_BLOB_OFFSET_INVALID) || (err == ERR_BLOB_NOT_FOUND) ||
        0 == len || !retBuf) {
        // we tried to read unwritten block, so create
        // an empty block buffer to place the data
        fauxBytes = boost::make_shared<std::string>(maxObjectSizeInBytes, '\0');
        fauxBytes->replace(iOff, writeBytes->length(),
                           writeBytes->c_str(), writeBytes->length());
    } else {
        fds_assert(len == maxObjectSizeInBytes);
        // Need to copy retBut into a modifiable buffer since retBuf is owned
        // by AM and should not be modified here.
        // TODO(Andrew): Make retBuf a const
        fauxBytes = boost::make_shared<std::string>(retBuf->c_str(), retBuf->length());
        fauxBytes->replace(iOff, writeBytes->length(),
                           writeBytes->c_str(), writeBytes->length());
    }
    // Update the resp so the next in the chain can grab the buffer
    writeBytes = fauxBytes;
    return std::make_pair(ERR_OK, fauxBytes);
}

}  // namespace fds
