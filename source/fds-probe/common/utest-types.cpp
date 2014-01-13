/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <utest-types.h>

namespace fds {

JsObject *
UT_Thread::js_exec_obj(JsObjTemplate *templ)
{
    return this;
}

JsObject *
UT_Server::js_exec_obj(JsObjTemplate *templ)
{
    return this;
}

JsObject *
UT_Load::js_exec_obj(JsObjTemplate *templ)
{
    return this;
}

}  // namespace fds
