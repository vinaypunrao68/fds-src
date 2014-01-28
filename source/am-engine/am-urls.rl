/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <am-plugin.h>

namespace fds {

%%{
    machine url_fsm;
    write data;
}%%

#define MAX_VER_VAL                    (64)
#define MAX_URL_VAL                    (256)

AME_Request *
ame_http_parse_url(AMEngine *eng, AME_HttpReq *req)
{
    int            i, cs, len, res;
    AME_Request   *rq;
    unsigned char *p, *cur;
    unsigned char *pe, *eof;
    unsigned char  ver[MAX_VER_VAL], domain[MAX_URL_VAL];
    unsigned char  obj[MAX_URL_VAL], bucket[MAX_URL_VAL];

    rq  = NULL;
    eof = NULL;
    res = 0;
    cur = req->uri.data + req->uri.len;
    p   = req->uri.data;
    for (pe = p; !isspace(*pe) && pe < cur; pe++);

    %%{
        # Actions
        action fds_data_action {
            printf("Data action is called: %s\n", fpc);
        }
        action fds_ctrl_action {
            printf("Ctrl action is called: %s\n", fpc);
        }
        action fds_test_action {
            printf("Test action is called: %s\n", fpc);
        }
        action fds_ver_action {
            for (cur = fpc; *cur == '/' && cur != pe; cur++);
            for (i = 0; cur[i] != '/' && cur != pe && i < MAX_VER_VAL; i++) {
                ver[i] = cur[i];
            }
            ver[i] = '\0';
            // printf("Ver action is called: %s\n", ver);
        }
        action fds_domain_action {
            for (cur = fpc; *cur == '/' && cur != pe; cur++);
            for (i = 0; cur[i] != '/' && cur != pe && i < MAX_URL_VAL; i++) {
                domain[i] = cur[i];
            }
            domain[i] = '\0';
            // printf("Domain action is called: %s\n", domain);
        }
        action fds_bucket_action {
            for (cur = fpc; *cur == '/' && cur != pe; cur++);
            for (i = 0; cur[i] != '/' && cur != pe && i < MAX_URL_VAL; i++) {
                bucket[i] = cur[i];
            }
            bucket[i] = '\0';
            // printf("Bucket action is called: %s\n", bucket);
        }
        action fds_object_action {
            for (cur = fpc; *cur == '/' && cur != pe; cur++);
            for (i = 0; cur[i] != '/' && cur != pe && i < MAX_URL_VAL; i++) {
                obj[i] = cur[i];
            }
            obj[i] = '\0';
            // printf("Obj action is called: %s\n", obj);
        }

        # Grammar
        fds_ver   = 'v' digit+ '/'                       >fds_ver_action;
        reserve   = 'api' | 'test' | 'data' | fds_ver;
        domain    = (alnum+ -- reserve) '/'              >fds_domain_action;
        bucket    = (alnum+ -- reserve) '/'              >fds_bucket_action;
        object    = alnum+ '/'?                          >fds_object_action;
        ctrl_fmt  = any+;
        test_fmt  = any+;
        data_fmt  = domain? bucket object?;
        fds_ctrl  = 'api/'  %fds_ctrl_action (fds_ver)? (ctrl_fmt)?;
        fds_test  = 'test/' %fds_test_action (fds_ver)? (test_fmt)?;
        fds_data  = 'data/' %fds_data_action (fds_ver)? (data_fmt);

        main := '/'+ (fds_ctrl | fds_data | fds_test | bucket object?)?;

        write init;
        write exec;
    }%%
    return rq;
}

}  // namespace fds
