/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <node-work.h>

namespace fds {

JsObject *
NodeQualifyObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::cout << __FUNCTION__ << std::endl;
    return this;
}

JsObject *
NodeIntegrateObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::cout << __FUNCTION__ << std::endl;
    return this;
}

JsObject *
NodeUpgradeObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::cout << __FUNCTION__ << std::endl;
    return this;
}

JsObject *
NodeDownObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::cout << __FUNCTION__ << std::endl;
    return this;
}

JsObject *
NodeDeployObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::cout << __FUNCTION__ << std::endl;
    return this;
}

JsObject *
NodeInfoMsgObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::cout << __FUNCTION__ << std::endl;
    return this;
}

JsObject *
NodeWorkItemObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::cout << __FUNCTION__ << std::endl;
    return this;
}

JsObject *
NodeFunctionalObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    std::cout << __FUNCTION__ << std::endl;
    return this;
}

}  // namespace fds
