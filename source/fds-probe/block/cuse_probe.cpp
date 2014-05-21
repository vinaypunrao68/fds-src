/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#define FUSE_USE_VERSION 30

#include <linux/fs.h>
#include <stdlib.h>
#include <cuse_lowlevel.h>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <fds_module.h>
#include <fds_assert.h>
#include <fds-probe/fds_probe.h>

extern "C" {
static void
cuse_open(fuse_req_t req, struct fuse_file_info *fi)
{
    fuse_reply_open(req, fi);
}

static void
cuse_read(fuse_req_t req, size_t size, off_t off, struct fuse_file_info *fi)
{
    size_t              rd_size;
    fds_uint64_t        vid;
    fds::ObjectID       oid;
    fds::ProbeMod       *mod;
    fds::ProbeIORequest *probe;

    mod = (fds::ProbeMod *)fuse_req_userdata(req);
    fds_verify(mod != nullptr);

    // Hardcode vid to 2 for now.
    vid   = 2;
    probe = mod->pr_alloc_req(&oid, 0, vid, off, size, nullptr);
    fds_verify(probe != nullptr);

    mod->pr_enqueue(probe);
    mod->pr_intercept_request(probe);
    mod->pr_get(probe);
    mod->pr_verify_request(probe);
    probe->req_wait();

    fuse_reply_buf(req, probe->pr_rd_buf(&rd_size), size);
    delete probe;
}

static void
cuse_write(fuse_req_t            req,
           const char            *buf,
           size_t                size,
           off_t                 off,
           struct fuse_file_info *fi)
{
    fds_uint64_t        vid;
    fds::ObjectID       oid;
    fds::ProbeMod       *mod;
    fds::ProbeIORequest *probe;

    mod = (fds::ProbeMod *)fuse_req_userdata(req);
    fds_verify(mod != nullptr);

    vid   = 2;
    probe = mod->pr_alloc_req(&oid, 0, vid, off, size, buf);
    fds_verify(probe != nullptr);

    mod->pr_enqueue(probe);
    mod->pr_intercept_request(probe);
    mod->pr_put(probe);
    mod->pr_verify_request(probe);
    probe->req_wait();

    fuse_reply_write(req, size);
    delete probe;
}

static void
cuse_ioctl(fuse_req_t            req,
           int                   cmd,
           void                  *arg,
           struct fuse_file_info *fi,
           unsigned              flags,
           const void            *in_buf,
           size_t                in_bufsz,
           size_t                out_bufsz)
{
    fds_uint32_t s;
    fds_uint64_t s64;

    switch (cmd) {
    case BLKGETSIZE64:
    case BLKGETSIZE:
        s = 1 << 30;
        if (!out_bufsz) {
            struct iovec iov = { arg, sizeof(s) };
            fuse_reply_ioctl_retry(req, NULL, 0, &iov, 1);
            break;
        }
        fuse_reply_ioctl(req, 0, reinterpret_cast<void *>(&s), sizeof(s));
        break;

    case BLKPBSZGET:
    case BLKSSZGET:
        s = 4 << 10;
        if (!out_bufsz) {
            struct iovec iov = { arg, sizeof(s) };
            fuse_reply_ioctl_retry(req, NULL, 0, &iov, 1);
            break;
        }
        fuse_reply_ioctl(req, 0, reinterpret_cast<void *>(&s), sizeof(s));
        break;

    case BLKROGET:
    case BLKDISCARDZEROES:
        s = 0;
        if (!out_bufsz) {
            struct iovec iov = { arg, sizeof(s) };
            fuse_reply_ioctl_retry(req, NULL, 0, &iov, 1);
            break;
        }
        fuse_reply_ioctl(req, 0, reinterpret_cast<void *>(&s), sizeof(s));
        break;

    default:
        fuse_reply_ioctl(req, 0, NULL, 0);
    }
}

static const struct cuse_lowlevel_ops cuse_clop =
{
    .init      = nullptr,
    .init_done = nullptr,
    .destroy   = nullptr,
    .open      = cuse_open,
    .read      = cuse_read,
    .write     = cuse_write,
    .flush     = nullptr,
    .release   = nullptr,
    .fsync     = nullptr,
    .ioctl     = cuse_ioctl,
    .poll      = nullptr
};

static const struct fuse_opt cuse_opts[] =
{
    { "--major=%u", 0, 1 },
    { "--minor=%u", 0, 1 },
    { "--dev-name=%s", 0, 1 },
    FUSE_OPT_KEY("-h", 0),
    FUSE_OPT_KEY("--help", 0),
    FUSE_OPT_END
};

// cuse_process_arg
// ----------------
//
static int
cuse_process_arg(void *data, const char *arg, int key, struct fuse_args *out)
{
    return 1;
}
}  // extern "C"

namespace fds {

ProbeMainLib gl_probeBlkLib("Main Probe Lib");

ProbeMainLib::ProbeMainLib(char const *const name)
    : Module(name), dev_argv{ dev_name, nullptr }, fuse_argc(0)
{
}

ProbeMainLib::~ProbeMainLib()
{
}

// mod_init
// --------
//
int
ProbeMainLib::mod_init(SysParams const *const param)
{
    namespace po = boost::program_options;

    std::string             devname;
    po::variables_map       vm;
    po::options_description desc("Probe Module Command Line Options");

    Module::mod_init(param);
    desc.add_options()
        ("help,h", "Show this help text")
        ("major", po::value<unsigned int>(&dev_major)->default_value(1000),
            "Set the device major number")
        ("minor", po::value<unsigned int>(&dev_minor)->default_value(0),
            "Set the device minior number")
        ("dev-name", po::value<std::string>(&devname)->default_value("fblk"),
            "Set the device name");

    po::store(po::command_line_parser(param->p_argc, param->p_argv).
              options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }
    if (devname.empty()) {
        std::cout << "Expect valid device name in --dev-name option"
            << std::endl;
        return 1;
    }
    snprintf(dev_name, sizeof(dev_name), "DEVNAME=%s", devname.c_str());
    return 0;
}

// mod_startup
// -----------
//
void
ProbeMainLib::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
ProbeMainLib::mod_shutdown()
{
}

// probe_run_main
// --------------
//
void
ProbeMainLib::probe_run_main(ProbeMod *adapter, bool thr)
{
    struct cuse_info ci;
    struct fuse_args fargs =
        FUSE_ARGS_INIT(mod_params->p_argc, mod_params->p_argv);

    if (fuse_opt_parse(&fargs, nullptr, cuse_opts, cuse_process_arg)) {
        std::cout << "Failed to parse option\n" << std::endl;
        exit(1);
    }
    memset(&ci, 0, sizeof(ci));
    ci.flags         = CUSE_UNRESTRICTED_IOCTL;
    ci.dev_major     = dev_major;
    ci.dev_minor     = dev_minor;
    ci.dev_info_argc = 1;
    ci.dev_info_argv = dev_argv;
    cuse_lowlevel_main(fargs.argc, fargs.argv, &ci, &cuse_clop, adapter);
}

}  // namespace fds
