/* Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_UUID_H_
#define SOURCE_INCLUDE_FDS_UUID_H_

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

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

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_UUID_H_
