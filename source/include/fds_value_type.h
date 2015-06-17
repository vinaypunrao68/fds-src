/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_VALUE_TYPE_H_
#define SOURCE_INCLUDE_FDS_VALUE_TYPE_H_

namespace fds
{

/**
 * This template wraps a basic (intrinsic) type and creates a new
 * non-implicitly convertable type with that explicit (get()) method to
 * retrieve the intrinsic type.
 *
 * Use-cases include needing to create a new type that has specific overload
 * operators or streaming functions; e.g. fds_volid_t
 */
template<typename T>
struct fds_value_type {
    using value_type = T;
    value_type v;

 public:
    fds_value_type() {}
    explicit fds_value_type(const value_type v_) : v(v_) {}
    fds_value_type(const fds_value_type & t_) = default;
    fds_value_type(fds_value_type&& t_) = default;
    fds_value_type& operator=(const fds_value_type& rhs) = default;
    fds_value_type& operator=(fds_value_type&& rhs) = default;
    fds_value_type& operator=(const value_type & rhs) { v = rhs; return *this;}
    bool operator==(const fds_value_type & rhs) const { return v == rhs.v; }
    bool operator!=(const fds_value_type & rhs) const { return !(operator==(rhs)); }
    bool operator<(const fds_value_type & rhs) const { return v < rhs.v; }
    bool operator>(const fds_value_type & rhs) const { return v > rhs.v; }

    value_type const& get() const { return v; }
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_VALUE_TYPE_H_
