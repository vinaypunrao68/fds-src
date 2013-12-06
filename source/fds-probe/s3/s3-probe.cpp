/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds-probe/s3-probe.h>
#include <am-plugin.h>
#include <fds_assert.h>

namespace fds {

ProbeS3Eng gl_probeS3Eng("S4 Probe Eng");

// ---------------------------------------------------------------------------
// GetObject Probe
// ---------------------------------------------------------------------------

Probe_GetObject::Probe_GetObject(AMEngine *eng, AME_HttpReq *req)
    : S3_GetObject(eng, req)
{
}

Probe_GetObject::~Probe_GetObject()
{
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

    cookie = ame_ctx->ame_alloc_buf(10, &buf, &len);
    ame_set_std_resp(0, 10);
    ame_send_response_hdr();
    ame_send_resp_data(cookie, 10, true);

    fds_assert(ame_ctx != NULL);
    ame_ctx->ame_notify_handler();
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

// PUT request handler
// -------------------
//
void
Probe_PutObject::ame_request_handler()
{
    int            len;
    char           *buf;
    fds_uint64_t   vid;
    ObjectID       oid;
    ProbeMod       *probe;
    ProbeIORequest *preq;

    // Get data from S3 connector.
    buf = ame_reqt_iter_data(&len);

    // Hookup with the back-end probe adapter.
    vid   = 2;
    probe = ((ProbeS3Eng *)ame)->probe_get_adapter();
    preq  = probe->pr_alloc_req(oid, 0, vid, 0, len, buf);
    fds_verify(preq != nullptr);

    probe->pr_enqueue(*preq);
    probe->pr_intercept_request(*preq);
    probe->pr_put(*preq);
    probe->pr_verify_request(*preq);

    // We're doing sync. call right now.
    delete preq;

    ame_set_std_resp(0, 10);
    ame_send_response_hdr();

    fds_assert(ame_ctx != NULL);
    ame_ctx->ame_notify_handler();
}

} // namespace fds
