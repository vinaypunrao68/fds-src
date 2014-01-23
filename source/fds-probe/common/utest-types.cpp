/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <iostream>
#include <utest-types.h>

namespace fds {

JsObject *
UT_Thread::js_exec_obj(JsObject *parent,
                       JsObjTemplate *templ, JsObjOutput *out)
{
    ut_thr_param_t *p;

    p = thr_param();
    std::cout << "Thread obj exec with: "
        << "min " << p->thr_min << ", max " << p->thr_max
        << ", spawn " << p->thr_spawn_thres << ", idle " << p->thr_idle_sec
        << std::endl;

    return this;
}

JsObject *
UT_Server::js_exec_obj(JsObject *parent,
                       JsObjTemplate *templ, JsObjOutput *out)
{
    ut_srv_param_t *p;

    p = srv_param();
    std::cout << "Server obj exec with: "
        << "port " << p->srv_port << std::endl;

    return this;
}

JsObject *
UT_Load::js_exec_obj(JsObject *parent,
                     JsObjTemplate *templ, JsObjOutput *out)
{
    ut_load_param_t *p;

    p = load_param();
    std::cout << "Load obj exec with: "
        << "rt loop " << p->rt_loop << ", rt_concurrent " << p->rt_concurrent
        << ", rt_duration " << p->rt_duration_sec
        << std::endl;

    return this;
}

JsObject *
UT_RunSetup::js_exec_obj(JsObject *parent,
                         JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "Run-Setup exec is called" << std::endl;
    return JsObject::js_exec_obj(this, templ, out);
}

JsObject *
UT_ServerLoad::js_exec_obj(JsObject *parent,
                           JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "Server load obj " << std::endl;
    return JsObject::js_exec_obj(this, templ, out);
}

JsObject *
UT_ClientLoad::js_exec_obj(JsObject *parent,
                           JsObjTemplate *templ, JsObjOutput *out)
{
    int                i, num;
    UT_ClientLoad     *obj;
    ut_client_param_t *p;

    num = js_array_size();
    for (i = 0; i < num; i++) {
        obj = static_cast<UT_ClientLoad *>((*this)[i]);
        p   = obj->client_load_param();
        std::cout << "Client load " << i << ", script: "
            << p->cl_script << ", args "
            << p->cl_script_args << std::endl;
    }
    std::cout << "Client load obj " << std::endl;
    return this;
}

JsObject *
UT_RunInput::js_exec_obj(JsObject *parent,
                         JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "RunInput exec " << std::endl;
    return JsObject::js_exec_obj(this, templ, out);
}

}  // namespace fds
