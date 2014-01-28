/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_FDS_NET_FDS_NET_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_FDS_NET_FDS_NET_PROBE_H_

#include <stdio.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <string>

#include <fds-probe/fds_probe.h>

class netSessionTbl;

namespace FDS_ProtocolInterface {
class FDSP_DataPathReqClient;
}

namespace fds {
class exampleDataPathRespIf;
class fdsNetProbeMod : public ProbeMod {
  public:
    fdsNetProbeMod(char const *const name,
                   probe_mod_param_t *param,
                   Module *owner);
    virtual ~fdsNetProbeMod() {}

    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    fdsNetProbeMod *pr_new_instance();

  private:
    std::string remoteIp;
    std::string localIp;
    fds_uint32_t numChannels;

    boost::shared_ptr<netSessionTbl> amTbl;

    boost::shared_ptr<FDS_ProtocolInterface::FDSP_DataPathReqClient> dpClient;
    std::string sid;
    boost::shared_ptr<exampleDataPathRespIf> edpri;
};

// S3 FDS net Probe Adapter.
//
extern fdsNetProbeMod           gl_fdsNetProbeMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_FDS_NET_FDS_NET_PROBE_H_
