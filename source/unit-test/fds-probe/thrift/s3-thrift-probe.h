/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_S3_THRIFT_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_S3_THRIFT_PROBE_H_

/*
 * Header file Template for probe adapter.  Replace XX with your name space.
 */
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <ProbeService.h>

#include <fds-probe/fds_probe.h>

namespace fds {
using namespace std;                         // NOLINT
using namespace apache::thrift;              // NOLINT
using namespace apache::thrift::protocol;    // NOLINT
using namespace apache::thrift::transport;   // NOLINT

class Thrift_ProbeMod : public ProbeMod
{
  public:
    Thrift_ProbeMod(char const *const name,
                    probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner), rpc_client(NULL) {}
    virtual ~Thrift_ProbeMod() {}

    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    Thrift_ProbeMod *pr_new_instance();
  private:
    boost::shared_ptr<TTransport>  rpc_sock;
    boost::shared_ptr<TTransport>  rpc_trans;
    boost::shared_ptr<TProtocol>   rpc_proto;
    ProbeServiceClient            *rpc_client;
};

// S3Thrift Probe Adapter.
//
extern Thrift_ProbeMod           gl_thriftProbeMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_S3_THRIFT_PROBE_H_
