/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace XX with your namespace.
 */
#include <o.h>

namespace fds {

// -----------------------------------------------------------------------------------
// Replace it with your handlers
// -----------------------------------------------------------------------------------
JsObject *
SanjayObj::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    sanjay_in_t *in = sanjay_in();
    std::cout << "Sanjay obj is called " << in->date << std::endl;
    return this;
}

JsObject *
BaoObj::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "Bao obj is called" << std::endl;
    return this;
}

JsObject *
VyObj::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "Vy obj is called" << std::endl;
    return this;
}

}  // namespace fds
