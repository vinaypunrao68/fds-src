/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace Thrpool with your namespace.
 */

#include <fds-probe-queue.h>
#include <platform/fds-shm-queue.h>
#include <list>
#include <string>
#include <iostream>

namespace fds {

probe_mod_param_t shm_queue_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

shm_queue_ProbeMod gl_shm_queue_ProbeMod("shm_queue Probe Adapter",
                           &shm_queue_probe_param, nullptr);


shm_queue_ProbeMod::

// pr_new_instance
// ---------------
//
ProbeMod *
shm_queue_ProbeMod::pr_new_instance()
{
    std::cout << "New instance!" << std::endl;

    shm_queue_ProbeMod *adapter =
            new shm_queue_ProbeMod("shm_queue Inst", &shm_queue_probe_param, nullptr);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
}

// pr_intercept_request
// --------------------
//
void
shm_queue_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
    std::cout << "Intercept request" << std::endl;
}

// pr_put
// ------
//
void
shm_queue_ProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
shm_queue_ProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
shm_queue_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
shm_queue_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
shm_queue_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
shm_queue_ProbeMod::mod_init(SysParams const *const param)
{
    std::cout << "Mod init!" << std::endl;
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
shm_queue_ProbeMod::mod_startup()
{
    std::cout << "Mod startup!" << std::endl;

    JsObjManager  *mgr;

    if (pr_parent != NULL) {
        std::cout << "Setting template." << std::endl;
        mgr = pr_parent->pr_get_obj_mgr();
        mgr->js_register_template(new QueueSetupTempl(mgr));
        mgr->js_register_template(new QueueWorkloadTempl(mgr));
    }
}

// mod_shutdown
// ------------
//
void
shm_queue_ProbeMod::mod_shutdown()
{
    std::cout << "Shutdown!" << std::endl;
}

// -----------------------------------------------------------------------------------
// Queue control
// -----------------------------------------------------------------------------------
JsObject *
QueueInfo::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "QueueSetup called!" << std::endl;
    queue_info_t *params = queue_info();

    fds::FdsShmQueue<int> q(params->name);
    q.shmq_alloc(params->size);

    if (q.empty()) {
        std::cout << "Queue is created and empty!" << std::endl;
    }
    return this;
}

JsObject *
QueuePush::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    queue_push_payload_t *payload = queue_push_payload();

    // Attach to queue
    fds::FdsShmQueue<int> q(payload->name);
    q.shmq_connect();
    // Push our value

    std::cout << "Pushing " << payload->value << " to " << payload->name << std::endl;
    q.shmq_enqueue(payload->value);

    return this;
}

JsObject *
QueuePop::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "Pop called" << std::endl;
    queue_pop_payload_t *payload = queue_pop_payload();

    fds::FdsShmQueue<int> q(payload->name);
    q.shmq_connect();
    int val(0);
    while (!q.shmq_dequeue(val)) {}

    std::cout << "Popped value = " << val << " from " << payload->name << std::endl;

    return this;
}

}  // namespace fds
