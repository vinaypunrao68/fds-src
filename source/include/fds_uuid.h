/* Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_UUID_H_
#define SOURCE_INCLUDE_FDS_UUID_H_

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <fds_types.h>

namespace fds {

/**
 * @brief Generates uuid (boost based)
 *
 * @return String uuid
 */
inline std::string get_uuid()
{
    boost::uuids::uuid u = boost::uuids::random_generator()();
    return boost::lexical_cast<std::string>(u);
}

/**
 * @brief Generates 64bit uuid from string
 *
 * Uses boost::uuids::string_generator, which generates 128bit value. 
 * This function returns low 64bit of that 128bit value.
 */
inline fds_uint64_t fds_get_uuid64(const std::string& str) {
    fds_uint64_t ret_val = 0;
    fds_uint64_t v64 = 0;
    // just making up some random namespace uuid
    boost::uuids::uuid namespace_uuid =
            {{0x2b, 0x39, 0x94, 0xde, 0xfa, 0x11, 0x00, 0x00,
             0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xcd, 0xef}};
    boost::uuids::name_generator gen(namespace_uuid);
    boost::uuids::uuid u = gen(str);

    // this method assumes boost uuid is at last 8 x uint8_t
    fds_verify(u.size() >= 8);

    boost::uuids::uuid::iterator const_it = u.begin();
    ret_val = *const_it++;
    for (fds_uint32_t shift = 8; shift < 64; shift+=8) {
        v64 = (*const_it++);
        ret_val |= (v64 << shift);
    }
    // TODO(Anna) since we are casting uint64 to int64 uuids at this
    // moment, set the most significant bit to 0, so that
    // we don't lose info when casting between signed and unsigned
    return (ret_val & 0x7FFFFFFFFFFFFFFF);
}

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_UUID_H_
