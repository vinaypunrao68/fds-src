/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_

#include <net/net-service.h>
#include <fds_typedefs.h>
#include <ProbeService.h>
#include <ProbeServiceAM.h>

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

class ProbePokeSvc : public EpSvc
{
  public:
    virtual ~ProbePokeSvc() {}
    ProbePokeSvc(const ResourceUUID &m, fds_uint32_t maj, fds_uint32_t min)
        : EpSvc(m, maj, min) {}

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

class ProbeTestSM_RPC;
class ProbeTestAM_RPC;

/**
 * The same probe EP but make it behaves like endpoint of an AM.
 */
class ProbeEpTestAM : public Module
{
  public:
    virtual ~ProbeEpTestAM() {}
    explicit ProbeEpTestAM(const char *name) : Module(name) {}

    // Module methods.
    int  mod_init(SysParams const *const p);
    void mod_startup();
    void mod_shutdown();

  protected:
    // One endpoint bound to a physical port of the local node.  Full duplex with
    // its peer.
    //
    EndPoint<fpi::ProbeServiceClient, ProbeTestAM_RPC>::pointer probe_ep;
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
    int  mod_init(SysParams const *const p);
    void mod_startup();
    void mod_shutdown();

  protected:
    // Handle to a service used to pass message to it.
    //
    EpSvcHandle::pointer     am_hello;
    EpSvcHandle::pointer     am_bye;
    EpSvcHandle::pointer     am_poke;
};

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
    void mod_startup();
    void mod_shutdown();

  protected:
    // Many services built on top of the endpoint.
    //
    ProbeHelloSvc            *svc_hello;
    ProbeByeSvc              *svc_bye;
    ProbePokeSvc             *svc_poke;

    // One endpoint bound to a physical port of the local node.  Full duplex with
    // its peer.
    //
    EndPoint<fpi::ProbeServiceAMClient, ProbeTestSM_RPC>::pointer probe_ep;
};

extern ProbeEpTestSM         gl_ProbeTestSM;
extern ProbeEpTestAM         gl_ProbeTestAM;

}  // namespace fds
#endif  // SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_
