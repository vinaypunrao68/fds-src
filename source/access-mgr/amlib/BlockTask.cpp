/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "connector/BlockTask.h"

#include <boost/make_shared.hpp>

namespace fds
{

BlockTask::BlockTask(uint64_t const hdl) :
    handle(hdl)
{
}

void
BlockTask::handleReadResponse(std::vector<boost::shared_ptr<std::string>>& buffers,
                              boost::shared_ptr<std::string>& empty_buffer,
                              uint32_t len,
                              uint32_t const maxObjectSizeInBytes) {
    // acquire the buffers
    bufVec.swap(buffers);

    // Fill in any missing wholes with zero data, this is a special *block*
    // semantic for NULL objects.
    for (auto& buf: bufVec) {
        if (!buf || 0 == buf->size()) {
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

}  // namespace fds
