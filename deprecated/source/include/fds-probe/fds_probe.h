/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PROBE_FDS_PROBE_H_
#define SOURCE_INCLUDE_FDS_PROBE_FDS_PROBE_H_

#include <list>
#include <string>
#include <fds_module.h>
#include <fds_assert.h>
#include <fds_request.h>
#include <fds-probe/fds_err_inj.h>
#include <fds-probe/js-object.h>
#include <concurrency/ThreadPool.h>

/*
 * -------------------
 * Theory of Operation
 * -------------------
 * This header file describes the architecture of FDS Probe Module.
 *
 * I. Objectives:
 * ~~~~~~~~~~~~~~
 * - Hook up an FDS module/SW stack with industry standard test tools.
 *   In IO path, test tools can be block, object, or S3 API test suites.
 * - Reduce the time component owner spending to write unit test code.
 * - Provide error injection framework and performance stat collections.
 * - Isolate the testing component with the same load/workload test suites
 *   applied to the whole system.
 *
 * II. Block Diagram:
 * ~~~~~~~~~~~~~~~~~~
 *      FDS SW stack
 *                                         Industry tools/workload generator
 *  +------------------+                         +---------+-----------+
 *  :   Layers Above   :                         | Obj API | Block Dev |
 *  +------------------+     +---------------+   +---------+-----------+
 *  |  Testing Module  | <-> | Probe Adapter |<->|      FDS Probe      |
 *  +------------------+     +---------------+   +--------^-+----------+
 *  :   Layers Below   :    write probe adapter           | |
 *  +------------------+    instead of unit test  +-------+-v---------+
 *                                                |   FDS Probe CLI   |
 *                                                +-------------------+
 * III. IO Path Data Model:
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 * - External tools generate loads that map to 3 main operations: GET, PUT,
 *   and DELETE.
 * - The testing module receives requests for get/put/delete with common
 *   arugments in IO path: object ID, object offset, volume id, volume offset,
 *   data buffer and length.
 * - The probe request and probe adapter provide interface for the testing
 *   module to check-point the progress and/or inject errors to the execution
 *   path.
 * - If the testing module needs customized data beside IO arguments, it must
 *   provide additional methods and test codes:
 *   o Convert data inside the data buffer in "PUT" op to its own data type.
 *   o Convert its own data type to the serial format in the data buffer in
 *     "GET" op for external tools to save for later use.
 *   o Write test code in high level language to save/retrieve/coordinate
 *     the way customized data sets can be sent to the testing module.
 *
 * IV. Request Flow:
 * ~~~~~~~~~~~~~~~~~
 * FDS probe drives a request in GET, PUT, or DELETE operation in the following
 * order:
 * - Call the adapter module to allocate a probe request.
 * - Fill in probe request with data it receives.
 * - Call the adapter module to let it examine/intercept the request with data
 *   before sending it to GET/PUT/DELETE op.
 * - Call the adapter module to handle GET/PUT/DELETE op with the given
 *   request.
 * - The adapter module and testing module can use the probe request for:
 *   1) Report check-points to collect performance stats.
 *   2) Evaluate conditional triggers to activate error injection actions.
 *   3) Provide customized actions to run at injection points once a trigger
 *      is hit.
 * - The probe adapter must call the probe request's complete function when the
 *   request is done.
 * - The probe adapter must provide the method to verify the operation.
 * - The probe adapter must provide the method to generate report when asked.
 *
 * V. Error Injection Model:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 * Probe adapter can hook up with error injection module in FDS Probe to
 * intercept and inject errors to an execution path.  The error injection
 * module provides the following capabilities:
 * - Present all injection points provided by FDS modules.
 * - Present all generic trigger conditions provided by the injection framework
 *   and FDS modules.
 * - Present all generic actions to run when a trigger at an injection point
 *   hits.
 * - Present customized condition triggers and actions provided by each FDS
 *   module.
 * - Enable external script to attach conditional triggers and action to each
 *   injection point.
 *
 * Generic conditional triggers that could attach to any injection point:
 * - Fire the trigger after a percentage hits.
 * - Fire the trigger only after a global flag is set.
 * - Fire the trigger if payload data matches a magic pattern setup by CLI.
 * - Fire the trigger on special vol id, offset, object id, offset value.
 *
 * Generic actions that could attach to any injection point:
 * - Bail out the request flow back to FDS Probe Module.  Used this action to
 *   short circuit the testing module to unit test it w/out involving layers
 *   below it in the stack.
 * - Do delay to widen a race condition window, or causing timeout.
 * - Panic the program.
 * - Corrupt data inside the request.
 * - Control HW/OS flags to simulate failures.
 * - Control system resources such as disable network links, lower priority of
 *   the running process...
 *
 * To hookup with error injection, the probe adapter must provide:
 * - Method to list attributes of all injection points (name, id).
 * - Method for FDS Probe to know if it can attach a generic injection trigger
 *   to the injection point.  In case of customized injection trigger, provide
 *   the spec for CLI to setup conditional triggers.
 * - Method for FDS probe to know if it can attach a generic action to the
 *   injection point.  In case of customized action, provide the action
 *   name and injection points that it can attach the action to these points.
 */
namespace fds {

class ProbeMod;

// Generic request from FDS Probe Module to drive probe traffic to the testing
// module.
//
class ProbeRequest : public fdsio::Request
{
  public:
    ProbeRequest(int stat_cnt, size_t bufsize, ProbeMod *mod);
    virtual ~ProbeRequest();

    // pr_request_done
    // ---------------
    // The testing module must call this method when it's done with the current
    // request.
    //
    void pr_request_done();

    // pr_stat_begin
    // -------------
    // If the testing module carry this object along its path, it can call
    // these API to record stat and provide injection points.
    //
    void pr_stat_begin(int stat_idx);
    void pr_stat_end(int stat_id);
    void pr_inj_point(probe_point_e point);

    // pr_inj_eval_trigger
    // -------------------
    // Write custom conditional trigger to return true to fire the action
    // by evaluating run-time data set with shared data in FDS Probe and
    // owner module.
    //
    virtual bool
    pr_inj_eval_trigger(probe_point_e point, ProbeMod *probe, Module *owner)
    {
        return false;
    }

    // pr_inj_do_action
    // ----------------
    // Write custom action to run when the conditional trigger hits.
    //
    virtual void pr_inj_do_action(ProbeMod *mod, Module *owner) {}

    // pr_report_stat
    // --------------
    // Copy out the stat to caller who allocated the out array to accept
    // the data.
    //
    virtual bool pr_report_stat(probe_stat_rec_t *out, int rec_cnt);

    // pr_rd_buf
    // ---------
    // Return the pointer to the internal buffer used for read and its size.
    //
    const char *pr_rd_buf(size_t *size)
    {
        *size = pr_buf.getSize();
        return pr_buf.getData();
    }

  protected:
    friend class ProbeMod;
    ObjectBuf                pr_buf;
    int                      pr_stat_cnt;

    // pr_obj_from_data
    // ----------------
    // Construct user-defined data type from input buffer passed by load
    // generator.
    //
    void *
    pr_obj_from_data(ProbeMod *mod, Module *owner, ObjectBuf *buf)
    {
        return nullptr;
    }
    // pr_gen_usr_obj
    // --------------
    // Generate user-defined data type to the buffer stream to save for later
    // use.
    //
    virtual void
    pr_gen_usr_obj(ProbeMod *m, Module *owner, ObjectBuf *buf) {}


  private:
    ProbeMod                 *pr_mod;
    fds_uint64_t             *pr_stats;
};

// ----------------------------------------------------------------------------
// Probe request used in IO/data path.
// ----------------------------------------------------------------------------
class ProbeIORequest : public ProbeRequest
{
  public:
    ProbeIORequest(int           stat_cnt,
                   size_t        bufsize,
                   const char    *wr_buf,
                   ProbeMod      *mod,
                   ObjectID      *oid,
                   fds_uint64_t  off,
                   fds_uint64_t  vid,
                   fds_uint64_t  voff);

    virtual ~ProbeIORequest() {}

    ObjectID                 pr_oid;
    fds_volid_t              pr_vid;
    fds_uint64_t             pr_offset;
    fds_uint64_t             pr_voff;

    // Used for write/put because we don't want to have extra mem copy.
    size_t                   pr_wr_size;
    const char               *pr_wr_buf;
};

// ----------------------------------------------------------------------------
// Common Probe Adapter Interface
// ----------------------------------------------------------------------------

// Declare params as struct so that we can initialize it statically in tagged
// format.
//
typedef struct probe_mod_param probe_mod_param_t;
struct probe_mod_param
{
    int                      pr_stat_cnt;
    int                      pr_inj_pts_cnt;
    int                      pr_inj_act_cnt;
    fds_uint32_t             pr_max_sec_tout;
};

// Module owner must implement this Probe Adapter to hook it up to receive
// loads from standard unit test suites.
//
class ProbeMod : public Module
{
  public:
    ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner);
    virtual ~ProbeMod();

    /**
     * Common API to allocate IO request to use in data path.
     * @return the request used in required pure virtual functions.
     */
    virtual ProbeIORequest *
    pr_alloc_req(ObjectID      *oid,
                 fds_uint64_t  off,
                 fds_uint64_t  vid,
                 fds_uint64_t  voff,
                 size_t        buf_siz,
                 const char    *buf);

    /**
     * Allocate additional instance for this adapter so that the connector
     * can pump more data to this connector w/out doing any locking.
     * @return the adapter to hookup to the S3/block connector.
     */
    virtual ProbeMod *pr_new_instance() = 0;

    /**
     * Add other probe module to chain with this module.
     * @param adapter (i) - the module to chain.
     */
    virtual void pr_add_module(ProbeMod *chain);

    /**
     * Get the first available chained module that was added by the call above.
     * Block the caller if the list is empty.
     */
    virtual ProbeMod *pr_get_module();

    /**
     * Async API for the method above.
     * @param pool (i) thread pool to queue up the request when the chain
     *     list is empty.  NULL will block the caller.
     * @param chain (i) the module obj where its pr_get_module_callback()
     *     will be called.
     * @param req (i) request to pass to the callback method.
     */
    virtual void
    pr_get_module(fds_threadpool *pool, ProbeMod *chain, ProbeRequest *req);

    /**
     * Callback method for the async API above.
     */
    virtual void pr_get_module_callback(ProbeRequest *req);

    /**
     * Create threadpool for this probe module.
     */
    inline void
    pr_create_thrpool(int max_task, int spawn_thres,
                      int idle_sec, int min_thr, int max_thr)
    {
        fds_verify(pr_thrpool == NULL);
        pr_thrpool = new fds_threadpool(max_task,
                spawn_thres, idle_sec, min_thr, max_thr);
    }
    /**
     * Get threadpool from this probe module.
     */
    inline fds_threadpool *pr_get_thrpool() {
        return pr_thrpool;
    }

    // Module owner must provide the following functions to connect the module
    // to FDS Probe to receive workloads and hook up with standard front-end
    // unit test code.
    //
    // IMPORTANT: On every put/get/delete, the adapter must call
    // pr_request_done() to complete the request.
    //
    virtual void pr_intercept_request(ProbeRequest *req) = 0;
    virtual void pr_put(ProbeRequest *req) = 0;
    virtual void pr_get(ProbeRequest *req) = 0;
    virtual void pr_delete(ProbeRequest *req) = 0;
    virtual void pr_verify_request(ProbeRequest *req) = 0;
    virtual void pr_gen_report(std::string *out) = 0;

    // pr_enqueue
    // ----------
    // Enqueue the request to pending queue so that it can be called with
    // pr_request_done() for async code path.  The calling thread can block
    // waiting for the completion in req->req_wait().
    //
    virtual void
    pr_enqueue(ProbeRequest *req)
    {
        pr_queue.rq_enqueue(req, 0);
    }
    // pr_get_inj_points
    // -----------------
    // Return the list of injection points that this probe module provides.
    //
    virtual probe_inj_point_t const *const
    pr_get_inj_points(int *rec_cnt)
    {
        *rec_cnt = pr_param->pr_inj_act_cnt;
        return pr_inj_points;
    }
    // pr_get_inj_actions
    // ------------------
    // Return the list of actions used for each injection point provided by
    // this probe module.
    //
    virtual probe_inj_action_t const *const
    pr_get_inj_actions(int *rec_cnt)
    {
        *rec_cnt = pr_param->pr_inj_pts_cnt;
        return pr_inj_actions;
    }
    // pr_get_stat_info
    // ----------------
    // Return the string that maps stat id for caller to decode the data.
    //
    virtual probe_stat_info const *const
    pr_get_stat_info(int *rec_cnt)
    {
        *rec_cnt = pr_param->pr_stat_cnt;
        return pr_stats_info;
    }
    // pr_get_obj_mgr
    // --------------
    //
    inline JsObjManager *pr_get_obj_mgr()
    {
        return pr_objects;
    }
    inline Module *pr_get_owner_module()
    {
        return pr_mod_owner;
    }

  protected:
    friend class ProbeRequest;
    fdsio::RequestQueue      pr_queue;
    std::list<ProbeMod *>    pr_chain;
    ProbeMod                 *pr_parent;
    JsObjManager             *pr_objects;

    fds_mutex                pr_mtx;
    fds_threadpool           *pr_thrpool;
    probe_mod_param_t        *pr_param;
    Module                   *pr_mod_owner;
    probe_stat_info_t        *pr_stats_info;
    probe_inj_point_t        *pr_inj_points;
    probe_inj_action_t       *pr_inj_actions;

    // Common code to evalue an injection point.
    //
    virtual void pr_inj_point(probe_point_e probe, ProbeRequest *req);

    // Generic injection point triggers.
    virtual bool pr_inj_trigger_pct_hit(probe_point_e inj, ProbeRequest *req);
    virtual bool pr_inj_trigger_rand_hit(probe_point_e inj, ProbeRequest *);
    virtual bool pr_inj_trigger_freq_hit(probe_point_e inj, ProbeRequest *);
    virtual bool pr_inj_trigger_payload(probe_point_e inj, ProbeRequest *);
    virtual bool pr_inj_trigger_io_attr(probe_point_e inj, ProbeRequest *);

    // Generic injection actions.
    //
    virtual void pr_inj_act_bailout(ProbeRequest *req);
    virtual void pr_inj_act_panic(ProbeRequest *req);
    virtual void pr_inj_act_delay(ProbeRequest *req);
    virtual void pr_inj_act_corrupt(ProbeRequest *req);
};

// ----------------------------------------------------------------------------
// Probe Adapters for Admin/control path.
// ----------------------------------------------------------------------------
class ProbeCtrlLoad : public ProbeMod
{
  public:
    ProbeCtrlLoad();
    ~ProbeCtrlLoad();

  protected:
};

// ----------------------------------------------------------------------------
// Probe Block Library Module
// ----------------------------------------------------------------------------
class ProbeMainLib : public Module
{
  public:
    explicit ProbeMainLib(char const *const name);
    ~ProbeMainLib();

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    void probe_run_main(ProbeMod *adapter, bool thr = false);
  private:
    unsigned int             dev_major;
    unsigned int             dev_minor;
    char                     dev_name[128];
    const char               *dev_argv[2];

    int                      fuse_argc;
    char                     *fuse_argv[4];
};

extern ProbeMainLib          gl_probeBlkLib;

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROBE_FDS_PROBE_H_
