/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_ALWAYS_CALL_H_
#define SOURCE_INCLUDE_UTIL_ALWAYS_CALL_H_

namespace fds
{

template<typename C>
struct always_call {
    always_call() = delete;
    explicit always_call(C const&& call) :
        c(call)
    {}
    always_call(always_call const& rhs) = delete;
    always_call& operator=(always_call const& rhs) = delete;
    always_call(always_call && rhs) = default;
    always_call& operator=(always_call && rhs) = default;
    ~always_call()
    { c(); }
    C c;
};

using nullary_always = always_call<std::function<void()>>;

}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_ALWAYS_CALL_H_
