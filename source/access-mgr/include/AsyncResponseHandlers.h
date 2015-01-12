/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_ASYNCRESPONSEHANDLERS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_ASYNCRESPONSEHANDLERS_H_

#include <functional>
#include "responsehandler.h"

namespace fds
{

template<typename T>
struct AsyncResponseHandler :   public ResponseHandler,
                                public T
{
    typedef std::function<void (T*, Error const&)> proc_type;

    explicit AsyncResponseHandler(proc_type _proc_func)
        : proc_func(_proc_func)
    { type = HandlerType::IMMEDIATE; }

    void process()
    { fds_assert(proc_func); proc_func(static_cast<T*>(this), error); }

    proc_type proc_func;
};

}  // namespace fds


#endif  // SOURCE_ACCESS_MGR_INCLUDE_ASYNCRESPONSEHANDLERS_H_
