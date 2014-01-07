/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds-probe/s3-probe.h>
#include <util/fds_stat.h>
#include <am-plugin.h>
#include <fds_assert.h>
#include <sh_app.h>

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace fds {

ProbeS3Eng gl_probeS3Eng("S4 Probe Eng");

ProbeS3Eng::ProbeS3Eng(char const *const name) : AMEngine_S3(name)
{
    pr_thrpool = new fds_threadpool(10);
}

ProbeS3Eng::~ProbeS3Eng()
{
    delete pr_thrpool;
}

// ---------------------------------------------------------------------------
// GetObject Probe
// ---------------------------------------------------------------------------

Probe_GetObject::Probe_GetObject(AMEngine *eng, AME_HttpReq *req)
    : S3_GetObject(eng, req)
{
    ame_stat_pt = STAT_NGX_GET;
}

Probe_GetObject::~Probe_GetObject()
{
}

// ame_request_resume
// ------------------
//
int
Probe_GetObject::ame_request_resume()
{
    fds_stat_record(STAT_NGX,
                    STAT_NGX_GET_RESUME, ame_clk_fdsn, fds_rdtsc());
    return NGX_DONE;
}

// GET request handler
// -------------------
//
void
Probe_GetObject::ame_request_handler()
{
    int        len;
    char      *buf;
    ame_buf_t *cookie;

    if (get_bucket_id() == "stat") {
        cookie = ame_ctx->ame_alloc_buf(1 << 20, &buf, &len);
        len = gl_fds_stat.stat_out(STAT_NGX, buf, len);
    } else {
        cookie = ame_ctx->ame_alloc_buf(1024, &buf, &len);
    }
    ame_clk_fdsn = fds_rdtsc();
    fds_stat_record(STAT_NGX, STAT_NGX_GET_FDSN, ame_clk_all, ame_clk_fdsn);

    ame_set_std_resp(0, len);
    ame_send_response_hdr();
    ame_send_resp_data(cookie, len, true);

    fds_assert(ame_ctx != NULL);
    ame_ctx->ame_notify_handler();
    fds_stat_record(STAT_NGX,
                    STAT_NGX_GET_FDSN_RET, ame_clk_fdsn, fds_rdtsc());
}

// ---------------------------------------------------------------------------
// PutObject Probe
// ---------------------------------------------------------------------------

Probe_PutObject::Probe_PutObject(AMEngine *eng, AME_HttpReq *req)
    : S3_PutObject(eng, req)
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
                Probe_PutObject  *obj)
{
	const char *buf;
	size_t len;

//	will have to integrate this with  thrift 
//	shApp_interface *shAppPtr = new shApp_interface();

	buf = preq->pr_rd_buf(&len);
//	shAppPtr->app_send_data(buf, len);
	printf(" probe_obj_write: buf %p, len %d\n", buf, len);


    delete preq;
    obj->ame_signal_resume(NGX_HTTP_OK);
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
    int            len;
    char           *buf;
    fds_uint64_t   vid;
    ObjectID       oid(len, (fds_uint64_t)&len);
    ProbeMod       *probe;
    ProbeS3Eng     *s3eng;
    ProbeIORequest *preq;

    // Get data from S3 connector.
    buf = ame_reqt_iter_data(&len);
printf("ame_request_handler:buf %p, len %d\n", buf, len);

    // Hookup with the back-end probe adapter.
    vid   = 2;
    s3eng = static_cast<ProbeS3Eng *>(ame);
    probe = s3eng->probe_get_adapter();
    preq  = probe->pr_alloc_req(oid, 0, vid, 0, len, buf);

    fds_verify(preq != nullptr);
    s3eng->probe_get_thrpool()->schedule(probe_obj_write, probe, preq, this);
}

}   // namespace fds
