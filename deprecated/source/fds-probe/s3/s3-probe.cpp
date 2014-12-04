/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <list>
#include <string>
#include <fds-probe/s3-probe.h>
#include <util/fds_stat.h>
#include <util/timeutils.h>
#include <am-plugin.h>
#include <fds_assert.h>
#include <utest-types.h>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace fds {

static probe_mod_param_t s3_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

ProbeS3Eng gl_probeS3Eng("S4 Probe Eng");

ProbeS3Eng::ProbeS3Eng(char const *const name) : AMEngine_S3(name)
{
    probe_s3 = new ProbeS3(name, &s3_probe_param, NULL);
    probe_s3->pr_create_thrpool(-1, 10, 10, 10, 10);
}

ProbeS3Eng::~ProbeS3Eng()
{
    delete probe_s3;
}

ProbeS3::ProbeS3(char const *const name,
                 probe_mod_param_t *param, Module *owner)
    : ProbeMod(name, param, owner)
{
    pr_objects->js_register_template(new UT_RunSetupTempl(pr_objects));
    pr_objects->js_register_template(new UT_RunInputTemplate(pr_objects));
}

ProbeS3::~ProbeS3()
{
}

// ---------------------------------------------------------------------------
// GetObject Probe
// ---------------------------------------------------------------------------
Probe_GetObject::Probe_GetObject(AMEngine *eng, AME_HttpReq *req)
    : S3_GetObject(eng, req), preq(NULL)
{
    ame_stat_pt = STAT_NGX_GET;
}

Probe_GetObject::~Probe_GetObject()
{
}

// probe_obj_read
// --------------
//
static void
probe_obj_read(ProbeMod        *probe,
               ProbeIORequest  *preq,
               Probe_GetObject *obj,
               ProbeS3Eng      *s3eng)
{
    obj->ame_clk_fdsn_cb = fds::util::getClockTicks();
    fds_stat_record(STAT_NGX, STAT_NGX_GET_FDSN_CB,
                    obj->ame_clk_fdsn, obj->ame_clk_fdsn_cb);

    probe->pr_enqueue(preq);
    probe->pr_intercept_request(preq);
    probe->pr_get(preq);
    probe->pr_verify_request(preq);
    preq->pr_request_done();

    obj->ame_signal_resume(NGX_HTTP_OK);
    s3eng->probe_add_adapter(probe);
}

// ame_request_resume
// ------------------
//
int
Probe_GetObject::ame_request_resume()
{
    fds_uint32_t len;
    char      *adr;
    ame_buf_t *buf;

    fds_stat_record(STAT_NGX,
                    STAT_NGX_GET_RESUME, ame_clk_fdsn_cb, fds::util::getClockTicks());

    fds_assert(ame_ctx != NULL);
    len = this->preq->pr_wr_size;
    ame_ctx->ame_update_output_buf(len);

    adr = ame_ctx->ame_curr_output_buf(&buf, &len);
    ame_set_std_resp(0, len);
    ame_send_response_hdr();
    ame_send_resp_data(buf, len, true);

    delete this->preq;
    return NGX_DONE;
}

// GET request handler
// -------------------
//
void
Probe_GetObject::ame_request_handler()
{
    int             len;
    fds_uint64_t    vid;
    ObjectID        oid(reinterpret_cast<uint8_t *> (&len));
    char           *buf;
    ame_buf_t      *cookie;
    ProbeMod       *probe;
    ProbeS3Eng     *s3eng;

    len = 4096;
    if (get_bucket_id() == "stat") {
        cookie = ame_ctx->ame_alloc_buf(1 << 20, &buf, &len);
        len = gl_fds_stat.stat_out(STAT_NGX, buf, len);
        ame_ctx->ame_update_output_buf(len);
        ame_set_std_resp(0, len);
        ame_send_response_hdr();
        ame_send_resp_data(cookie, len, true);
        return;
    } else {
        cookie = ame_ctx->ame_alloc_buf(len, &buf, &len);
    }
    ame_clk_fdsn = fds::util::getClockTicks();
    fds_stat_record(STAT_NGX, STAT_NGX_GET_FDSN, ame_clk_all, ame_clk_fdsn);
    ame_ctx->ame_push_output_buf(cookie);

    // Hookup with the back-end probe adapter.
    vid   = 2;
    s3eng = static_cast<ProbeS3Eng *>(ame);
    probe = s3eng->probe_get_adapter();
    this->preq = probe->pr_alloc_req(&oid, 0, vid, 0, len, buf);

    fds_verify(this->preq != NULL);
    s3eng->probe_get_thrpool()->
        schedule(probe_obj_read, probe, this->preq, this, s3eng);
}

// ---------------------------------------------------------------------------
// PutObject Probe
// ---------------------------------------------------------------------------
Probe_PutObject::Probe_PutObject(AMEngine *eng, AME_HttpReq *req)
    : S3_PutObject(eng, req), preq(NULL)
{
}

Probe_PutObject::~Probe_PutObject()
{
}

// probe_obj_write
// ---------------
//
static void
probe_obj_write(ProbeMod         *probe,
                ProbeIORequest   *preq,
                Probe_PutObject  *obj,
                ProbeS3Eng       *s3eng)
{
    probe->pr_enqueue(preq);
    probe->pr_intercept_request(preq);
    probe->pr_put(preq);
    probe->pr_verify_request(preq);
    preq->pr_request_done();
    delete preq;

    obj->ame_signal_resume(NGX_HTTP_OK);
    s3eng->probe_add_adapter(probe);
}

// ame_request_resume
// ------------------
//
int
Probe_PutObject::ame_request_resume()
{
    // Temp. code, we need to have better buffer managemnet.
    if (ame_ctx->ame_temp_buf != NULL) {
        ngx_pfree(ame_req->pool, ame_ctx->ame_temp_buf);
    }
    ame_finalize_response(ame_resp_status);
    return NGX_DONE;
}

// PUT request handler
// -------------------
//
void
Probe_PutObject::ame_request_handler()
{
    fds_uint32_t   len;
    char           *buf;
    fds_uint64_t   vid;
    ObjectID        oid(reinterpret_cast<uint8_t *> (&len));
    ProbeMod       *probe;
    ProbeS3Eng     *s3eng;
    ProbeIORequest *preq;

    // Get data from S3 connector.
    buf = ame_reqt_iter_data(&len);

    // Hookup with the back-end probe adapter.
    vid   = 2;
    s3eng = static_cast<ProbeS3Eng *>(ame);
    probe = s3eng->probe_get_adapter();
    preq  = probe->pr_alloc_req(&oid, 0, vid, 0, len, buf);

    fds_verify(preq != nullptr);
    s3eng->probe_get_thrpool()->
        schedule(probe_obj_write, probe, preq, this, s3eng);
}

// ---------------------------------------------------------------------------
// Probe PUT Bucket - used in control path.
// ---------------------------------------------------------------------------
Probe_PutBucket::Probe_PutBucket(AMEngine *eng, AME_HttpReq *req)
    : S3_PutBucket(eng, req)
{
}

Probe_PutBucket::~Probe_PutBucket()
{
}

// ctrl_obj_write
// --------------
//
static void
ctrl_obj_write(ProbeS3Eng *s3eng, ProbeS3 *clnt, Probe_PutBucket *put, char *buf)
{
    int                got, len;
    json_t            *root;
    char              *cur;
    AME_Ctx           *ctx;
    ame_buf_t         *mem;
    JsObjManager      *objs;
    json_error_t       err;
    JsObjOutput        out;

    objs = s3eng->probe_get_obj_mgr();
    root = json_loads(buf, 0, &err);
    if (root != NULL) {
        out.js_set_context(clnt);
        objs->js_exec(root, &out);
        json_decref(root);
    }
    s3eng->probe_add_adapter(clnt);

    ctx = put->ame_get_context();
    mem = ctx->ame_alloc_buf(1 << 20, &cur, &got);
    ctx->ame_push_output_buf(mem);

    std::list<std::string>::iterator it = out.js_output_init();
    len = out.js_out(&it, cur, got);
    mem->last = mem->pos + len;

    put->ame_signal_resume(NGX_HTTP_OK);
}

// Admin/control handler
// ---------------------
//
int
Probe_PutBucket::ame_request_resume()
{
    fds_uint32_t len;
    char      *adr;
    ame_buf_t *buf;

    adr = ame_ctx->ame_curr_output_buf(&buf, &len);
    if (len > 0) {
        ame_finalize = false;
        ame_set_std_resp(NGX_HTTP_OK, len);
        ame_send_response_hdr();
        ame_send_resp_data(buf, len, true);
    } else {
        ame_finalize_response(NGX_HTTP_OK);
    }
    return NGX_DONE;
}

void
Probe_PutBucket::ame_request_handler()
{
    fds_uint32_t len;
    char         *buf;
    ProbeS3      *clnt;
    ProbeS3Eng   *s3p;

    buf = ame_reqt_iter_data(&len);
    if (buf == NULL) {
        ame_signal_resume(NGX_HTTP_OK);
        return;
    }
    buf[len] = '\0';
    s3p  = static_cast<ProbeS3Eng *>(ame);
    clnt = static_cast<ProbeS3 *>(s3p->probe_get_adapter());
    s3p->probe_get_thrpool()->schedule(ctrl_obj_write, s3p, clnt, this, buf);
}

}   // namespace fds
