/*
 * scst/ScstMode.cpp
 *
 * Copyright (c) 2015, Brian Szmyd <szmyd@formationds.com>
 * Copyright (c) 2015, Formation Data Systems
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "connector/scst/ScstMode.h"
#include "connector/scst/ScstTask.h"

namespace fds {

CachingModePage::CachingModePage()
{
    std::memset(this, '\0', sizeof(CachingModePage));
    _header._page_code = 0x08;
    _header._length = sizeof(CachingModePage) - 2;
}

void CachingModePage::setPrefetches(size_t const minimum,
                                    size_t const maximum,
                                    size_t const ceiling,
                                    size_t const segment_size)
{
    _cache_segments = 1;
    _minimum_prefetch = htobe16(std::min((size_t)UINT16_MAX, minimum));
    _maximum_prefetch = htobe16(std::min((size_t)UINT16_MAX, maximum));
    _maximum_prefetch_ceiling = htobe16(std::min((size_t)UINT16_MAX, ceiling));
    _cache_segment_size = htobe16(std::min((size_t)UINT16_MAX, segment_size));
}

ControlModePage::ControlModePage()
{
    std::memset(this, '\0', sizeof(ControlModePage));
    _header._page_code = 0x0A;
    _header._length = sizeof(ControlModePage) - 2;
}

ModeHandler::ModeHandler()
{
}

void ModeHandler::setBlockDescriptor(size_t const lba_count, size_t const lba_size)
{
    *reinterpret_cast<uint32_t*>(&_block_descriptor._density_code) = htobe32(std::min(lba_count, (uint64_t)UINT_MAX));
    *reinterpret_cast<uint32_t*>(&_block_descriptor._reserved) = htobe32(lba_size);
}

void ModeHandler::writeModeParameters6(ScstTask* task, bool const block_descriptor, uint8_t const page_code)
{
    auto it = mode_pages.find(page_code);
    if (mode_pages.end() == it) {
        task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
        return;
    }
    auto& page = it->second;
    auto buffer = task->getResponseBuffer();
    auto to_write = task->getResponseBufferLen();
    size_t writing = 0;
    size_t written = 0;

    Mode6Header header;
    written = sizeof(Mode6Header); // Skip header till end

    // Write the Block Descriptor if requested
    if (block_descriptor) {
        header._block_descriptor_length = sizeof(BlockDescriptor);
        writing = std::min(to_write, sizeof(BlockDescriptor));
        memcpy(buffer + written, &_block_descriptor, writing);
        written += writing; to_write -= writing;
    }

    // Write the page
    writing = std::min(to_write, page.size());
    memcpy(buffer + written, page.data(), writing);
    written += writing; to_write -= writing;

    // Finally write the header
    header._header._data_length = written - 1;
    writing = std::min(task->getResponseBufferLen(), sizeof(Mode6Header));
    memcpy(buffer, &header, writing);

    task->setResponseLength(written);
}

void ModeHandler::writeModeParameters10(ScstTask* task, bool const block_descriptor, uint8_t const page_code)
{
    auto it = mode_pages.find(page_code);
    if (mode_pages.end() == it) {
        task->checkCondition(SCST_LOAD_SENSE(scst_sense_invalid_field_in_cdb));
        return;
    }
    auto& page = it->second;
    auto buffer = task->getResponseBuffer();
    auto to_write = task->getResponseBufferLen();
    size_t writing = 0;
    size_t written = 0;

    Mode10Header header;
    written = sizeof(Mode10Header); // Skip header till end

    // Write the Block Descriptor if requested
    if (block_descriptor) {
        header._block_descriptor_length = sizeof(BlockDescriptor);
        writing = std::min(to_write, sizeof(BlockDescriptor));
        memcpy(buffer + written, &_block_descriptor, writing);
        written += writing; to_write -= writing;
    }

    // Write the page
    writing = std::min(to_write, page.size());
    memcpy(buffer + written, page.data(), writing);
    written += writing; to_write -= writing;

    // Finally write the header
    header._data_length_hi = (written - 2) >> 8;
    header._header._data_length = written - 2;
    writing = std::min(task->getResponseBufferLen(), sizeof(Mode6Header));
    memcpy(buffer, &header, writing);

    task->setResponseLength(written);
}

}  // namespace fds
