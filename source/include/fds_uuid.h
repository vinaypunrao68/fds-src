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
    boost::uuids::uuid namespace_uuid;
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
    return ret_val;
}

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_UUID_H_
