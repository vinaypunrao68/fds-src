/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <am-engine/am-engine.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <shared/fds_types.h>
#include <fds_assert.h>
#include <am-plugin.h>

namespace fds {

const int    NGINX_ARG_PARAM = 80;
static char  nginx_prefix[NGINX_ARG_PARAM];
static char  nginx_config[NGINX_ARG_PARAM];
static char  nginx_signal[NGINX_ARG_PARAM];

static char const *nginx_start_argv[] =
{
    nullptr,
    "-p", nginx_prefix,
    "-c", nginx_config
};

static char const *nginx_signal_argv[] =
{
    nullptr,
    "-s", nginx_signal
};

AMEngine gl_AMEngine("AM Engine Module");

int
AMEngine::mod_init(SysParams const *const p)
{
    using namespace std;
    namespace po = boost::program_options;

    Module::mod_init(p);
    po::variables_map       vm;
    po::options_description desc("Access Manager Options");

    desc.add_options()
        ("help", "Show this help text")
        ("signal", po::value<std::string>(&eng_signal),
         "Send signal to access manager: stop, quit, reopen, reload");

    po::store(po::command_line_parser(p->p_argc, p->p_argv).
            options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    if (vm.count("help")) {
        cout << desc << endl;
        return 1;
    }
    if (vm.count("signal")) {
        if ((eng_signal != "stop") && (eng_signal != "quit") &&
            (eng_signal != "reopen") && (eng_signal != "reload")) {
            cout << "Expect --signal stop|quit|reopen|reload" << endl;
            return 1;
        }
    }
    return 0;
}

void
AMEngine::mod_startup()
{
}

// make_nginix_dir
// ---------------
// Make a directory that nginx server requies.
//
static void
make_nginx_dir(char const *const path)
{
    if (mkdir(path, 0755) != 0) {
        if (errno == EACCES) {
            std::cout << "Don't have permission to " << path << std::endl;
            exit(1);
        }
        fds_verify(errno == EEXIST);
    }
}

// run_server
// ----------
//
void
AMEngine::run_server()
{
    using namespace std;
    const string *fds_root;
    char          path[NGINX_ARG_PARAM];

    fds_root = &mod_params->fds_root;
    if (eng_signal != "") {
        nginx_signal_argv[0] = mod_params->p_argv[0];
        strncpy(nginx_signal, eng_signal.c_str(), NGINX_ARG_PARAM);
        ngx_main(FDS_ARRAY_ELEM(nginx_signal_argv), nginx_signal_argv);
        return;
    }
    strncpy(nginx_config, eng_conf, NGINX_ARG_PARAM);
    snprintf(nginx_prefix, NGINX_ARG_PARAM, "%s", fds_root->c_str());

    // Create all directories if they are not exists.
    umask(0);
    snprintf(path, NGINX_ARG_PARAM, "%s/%s", nginx_prefix, eng_logs);
    ModuleVector::mod_mkdir(path);
    snprintf(path, NGINX_ARG_PARAM, "%s/%s", nginx_prefix, eng_etc);
    ModuleVector::mod_mkdir(path);

    nginx_start_argv[0] = mod_params->p_argv[0];
    ngx_main(FDS_ARRAY_ELEM(nginx_start_argv), nginx_start_argv);
}

void
AMEngine::mod_shutdown()
{
}

AME_Request::AME_Request(ngx_http_request_t *req)
    : fdsio::Request(false)
{
}

AME_Request::~AME_Request()
{
}

void
AME_Request::ame_reqt_iter_reset()
{
}

ame_ret_e
AME_Request::ame_reqt_iter_next()
{
    return AME_OK;
}

char const *const
AME_Request::ame_reqt_iter_data(int *len)
{
    return NULL;
}

char const *const
AME_Request::ame_get_reqt_hdr_val(char const *const key)
{
    return NULL;
}

ame_ret_e
AME_Request::ame_set_resp_keyval(char const *const k, char const *const v)
{
    return AME_OK;
}

void *
AME_Request::ame_push_resp_data_buf(int *buf_len, char **buf_adr)
{
    return NULL;
}

void
AME_Request::ame_send_resp_data(void *buf_cookie, int val_len, int last_buf)
{
}

FDSN_GetObject::FDSN_GetObject(ngx_http_request_t *req)
    : AME_Request(req)
{
}

FDSN_GetObject::~FDSN_GetObject()
{
}

void
FDSN_GetObject::ame_request_handler()
{
}

S3_GetObject::S3_GetObject(ngx_http_request_t *req)
    : FDSN_GetObject(req),
      resp_xx_key(NULL), resp_xx_value(NULL)
{
}

S3_GetObject::~S3_GetObject()
{
}

ame_ret_e
S3_GetObject::ame_send_response_hdr()
{
    return AME_OK;
}

} // namespace fds
