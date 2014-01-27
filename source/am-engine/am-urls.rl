/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <am-plugin.h>

namespace fds {

%%{
    machine url_fsm;
    write data;
}%%

AME_Request *
ame_http_parse_url(AMEngine *eng, AME_HttpReq *req)
{
    int            cs;
    AME_Request   *rq;
    unsigned char *p;
    unsigned char *pe;

    p  = req->uri.data;
    pe = req->uri.data + req->uri.len;
    rq = NULL;

    %%{
        # Actions

        # Grammar
        bucket    = [0-9]+ "/";
        object    = bucket "/";

        main := (
            bucket |
            object
#            unit_test
        )*;

        write init;
        write exec;
    }%%
    return rq;
}

}  // namespace fds
