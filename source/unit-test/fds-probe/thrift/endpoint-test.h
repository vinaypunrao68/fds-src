/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_

#include <net/net-service.h>
#include <ProbeService.h>
#include <fds_typedefs.h>

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
    void ep_svc_down();

  protected:
    /* Customize data here to do error handling. */
};

/*
 * Many services that can be registered dynamically.
 */
class ProbeHelloSvc : public EpSvc
{
  public:
    /* Register service uuid 0xabcd, major ver 1, minor ver 1 */
    ProbeHelloSvc() : EpSvc(ResourceUUID(0xabcd), 1, 1) {}
    virtual ~ProbeHelloSvc() {}

    /* Whoever sends to uuid = 0xabcd, right message format will invoke this handler */
    void svc_receive_msg(const fpi::AsyncHdr &msg);

  private:
    /* Service private data structure here. */
};

class ProbeByeSvc : public EpSvc
{
  public:
    /* Register service uuid 0x1234, major ver 2, minor ver 1 */
    ProbeByeSvc() : EpSvc(ResourceUUID(0x1234), 2, 1) {}
    virtual ~ProbeByeSvc() {}

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

class ProbePokeSvc : public EpSvc
{
  public:
    /* Register service uuid 0xcafe, major ver 2, minor ver 5 */
    ProbePokeSvc() : EpSvc(ResourceUUID(0xcafe), 2, 5) {}
    virtual ~ProbePokeSvc() {}

    void svc_receive_msg(const fpi::AsyncHdr &msg);
};

/**
 * Endpoint test module.
 */
class ProbeEpTest : public Module
{
  public:
    explicit ProbeEpTest(const char *name);
    ~ProbeEpTest() {}

    // Module methods.
    //
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_shutdown();

    // Many services built on top of the endpoint.
    //
    ProbeHelloSvc            *svc_hello;
    ProbeByeSvc              *svc_bye;
    ProbePokeSvc             *svc_poke;

    // One endpoint bound to a physical port of the local node.  Full duplex with
    // its peer.
    //
    EndPoint<fpi::ProbeServiceClient, fpi::ProbeServiceIf>::pointer probe_ep;
};

extern ProbeEpTest            gl_ProbeTest;

}  // namespace fds
#endif  // SOURCE_UNIT_TEST_FDS_PROBE_THRIFT_ENDPOINT_TEST_H_
