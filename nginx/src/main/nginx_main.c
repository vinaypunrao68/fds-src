/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 *
 * Modified by Formation Data Systems, Inc.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>

extern int ngx_cdecl ngx_main(int argc, char *const *argv);

int ngx_cdecl
main(int argc, char *const *argv)
{
    return ngx_main(argc, argv);
}
