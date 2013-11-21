/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds-probe/s3-probe.h>

namespace fds {

// ---------------------------------------------------------------------------
// GetObject Probe
// ---------------------------------------------------------------------------

Probe_GetObject::Probe_GetObject(HttpRequest &req)
    : S3_GetObject(req)
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
    int    len;
    void   *cookie;
    char   *buf;

    cookie = fdsn_alloc_get_buffer(0, &buf, &len);
    fdsn_send_get_response(0, 0);
    fdsn_send_get_buffer(cookie, 0, true);
}

// ---------------------------------------------------------------------------
// PutObject Probe
// ---------------------------------------------------------------------------

Probe_PutObject::Probe_PutObject(HttpRequest &req)
    : S3_PutObject(req)
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
    fdsn_send_put_response(0);
}

} // namespace fds
