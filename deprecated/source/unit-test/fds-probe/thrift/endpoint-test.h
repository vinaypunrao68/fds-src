/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_

#include <net/net-service-tmpl.hpp>
#include <ProbeServiceSM.h>
#include <ProbeServiceAM.h>
#include <platform/net-plat-shared.h>
#include <net/BaseAsyncSvcHandler.h>

namespace fds {

/**
 * Error hahndling plugin to the endpoint error recovery model.
 */
class ProbeEpPlugin : public EpEvtPlugin
{
  public:
    ProbeEpPlugin() : EpEvtPlugin() {}
    virtual ~ProbeEpPlugin() {}

    void ep_connected();
    void ep_down();
    void svc_up(EpSvcHandle::pointer handle);
    void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

  protected:
    /* Customize data here to do error handling. */
};

/*
 * Many services that can be registered dynamically.
 */
class ProbeHelloSvc : public EpSvc
{
  public:
    virtual ~ProbeHelloSvc() {}
    ProbeHelloSvc(const ResourceUUID &m, fds_uint32_t maj, fds_uint32_t min)
        : EpSvc(m, maj, min) {}

    void svc_receive_msg(const fpi::AsyncHdr &msg);

  private:
    /* Service private data structure here. */
};

class ProbeByeSvc : public EpSvc
{
  public:
    virtual ~ProbeByeSvc() {}
    ProbeByeSvc(const ResourceUUID &m, fds_uint32_t maj, fds_uint32_t min)
        : EpSvc(m, maj, min) {}

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

/**
 * The same probe EP but make it behaves like endpoint of an AM.
 */
class ProbeEpTestAM : public Module
{
  public:
    virtual ~ProbeEpTestAM() {}
    explicit ProbeEpTestAM(const char *name) : Module(name) {}

    // Module methods.
    int  mod_init(SysParams const *const p) override;
    void mod_startup() override;
    void mod_enable_service() override;
    void mod_shutdown() override;

  protected:
};

/**
 * AM using services identified by uuids.
 */
class ProbeEpSvcTestAM : public Module
{
  public:
    virtual ~ProbeEpSvcTestAM() {}
    explicit ProbeEpSvcTestAM(const char *name) : Module(name) {}

    // Module methods.
    int  mod_init(SysParams const *const p) override;
    void mod_startup() override;
    void mod_shutdown() override;

  protected:
};

class ProbeTestServer;

/**
 * Probe Unit test EP, make it behaves like endpoint of an SM.
 */
class ProbeEpTestSM : public Module
{
  public:
    virtual ~ProbeEpTestSM() {}
    explicit ProbeEpTestSM(const char *name) : Module(name) {}

    // Module methods.
    int  mod_init(SysParams const *const p);
    void mod_startup() override;
    void mod_shutdown() override;
    void mod_enable_service() override;

  protected:
    // One endpoint bound to a physical port of the local node.  Full duplex with
    // its peer.
    //
    PlatNetEpPtr                        svc_ep;
    ProbeEpPlugin::pointer              svc_plugin;
    boost::shared_ptr<ProbeTestServer>  svc_recv;
};

extern ProbeEpTestSM         gl_ProbeTestSM;
extern ProbeEpTestAM         gl_ProbeTestAM;
extern ProbeEpSvcTestAM      gl_ProbeSvcTestAM;

}  // namespace fds
#endif  // SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_
