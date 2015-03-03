#ifndef __NET_SESS_RESP_CLIENT_H__
#define __NET_SESS_RESP_CLIENT_H__
#include <stdio.h>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include "fds_types.h"
#include <arpa/inet.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>

#include "fdsp/FDSP_ConfigPathReq.h"
#include "fdsp/FDSP_constants.h"
#include "fdsp/FDSP_DataPathResp.h"
#include "fdsp/FDSP_MetaDataPathResp.h"
#include "fdsp/FDSP_OMControlPathResp.h"
#include "fdsp/FDSP_types.h"
#include "fdsp/FDSP_MetaDataPathReq.h"
#include "fdsp/FDSP_SessionReq.h"
#include "fdsp/FDSP_DataPathReq.h"
#include "fdsp/FDSP_OMControlPathReq.h"


using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::concurrency;

using namespace std;
using namespace fds;
using namespace FDS_ProtocolInterface;

typedef void (*set_client_t)(const boost::shared_ptr<TTransport> transport, void* context);

class FdsDataPathReqProcessorFactory: public TProcessorFactory {
public:
  FdsDataPathReqProcessorFactory(const boost::shared_ptr<FDSP_DataPathReqIfFactory> &handlerFactory,
                                 set_client_t set_client_cb, void* context)
    : handler_factory(handlerFactory), set_client_hdlr(set_client_cb), context_(context) {}

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo& connInfo)
  {
    set_client_hdlr(connInfo.transport, context_);

    ReleaseHandler<FDSP_DataPathReqIfFactory> cleanup(handler_factory);
    boost::shared_ptr<FDSP_DataPathReqIf> handler(handler_factory->getHandler(connInfo), cleanup);
    boost::shared_ptr<TProcessor> processor(new FDSP_DataPathReqProcessor(handler));
    return processor;
  }

protected:
  boost::shared_ptr<FDSP_DataPathReqIfFactory> handler_factory;

private:
  set_client_t set_client_hdlr;
  void* context_;
};

class FdsMetaDataPathReqProcessorFactory: public TProcessorFactory {
public:
FdsMetaDataPathReqProcessorFactory(const boost::shared_ptr<FDSP_MetaDataPathReqIfFactory> &handlerFactory,
                                   set_client_t set_client_cb, void* context)
        : handler_factory(handlerFactory), set_client_hdlr(set_client_cb), context_(context) {}

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo& connInfo)
  {
    set_client_hdlr(connInfo.transport, context_);

    ReleaseHandler<FDSP_MetaDataPathReqIfFactory> cleanup(handler_factory);
    boost::shared_ptr<FDSP_MetaDataPathReqIf> handler(handler_factory->getHandler(connInfo), cleanup);
    boost::shared_ptr<TProcessor> processor(new FDSP_MetaDataPathReqProcessor(handler));
    return processor;
  }

protected:
  boost::shared_ptr<FDSP_MetaDataPathReqIfFactory> handler_factory;

private:
  set_client_t set_client_hdlr;
  void* context_;
};

class FdsConfigPathReqProcessorFactory: public TProcessorFactory {
public:
FdsConfigPathReqProcessorFactory(const boost::shared_ptr<FDSP_ConfigPathReqIfFactory> &handlerFactory,
                                 set_client_t set_client_cb, void* context)
        : handler_factory(handlerFactory), set_client_hdlr(set_client_cb), context_(context) {}

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo& connInfo)
  {
    set_client_hdlr(connInfo.transport, context_);

    ReleaseHandler<FDSP_ConfigPathReqIfFactory> cleanup(handler_factory);
    boost::shared_ptr<FDSP_ConfigPathReqIf> handler(handler_factory->getHandler(connInfo), cleanup);
    boost::shared_ptr<TProcessor> processor(new FDSP_ConfigPathReqProcessor(handler));
    return processor;
  }

protected:
  boost::shared_ptr<FDSP_ConfigPathReqIfFactory> handler_factory;

private:
  set_client_t set_client_hdlr;
  void* context_;
};

class FdsOMControlPathReqProcessorFactory: public TProcessorFactory {
public:
FdsOMControlPathReqProcessorFactory(const boost::shared_ptr<FDSP_OMControlPathReqIfFactory> &handlerFactory,
                                    set_client_t set_client_cb, void* context)
        : handler_factory(handlerFactory), set_client_hdlr(set_client_cb), context_(context) {}

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo& connInfo)
  {
    set_client_hdlr(connInfo.transport, context_);

    ReleaseHandler<FDSP_OMControlPathReqIfFactory> cleanup(handler_factory);
    boost::shared_ptr<FDSP_OMControlPathReqIf> handler(handler_factory->getHandler(connInfo), cleanup);
    boost::shared_ptr<TProcessor> processor(new FDSP_OMControlPathReqProcessor(handler));
    return processor;
  }

protected:
  boost::shared_ptr<FDSP_OMControlPathReqIfFactory> handler_factory;

private:
  set_client_t set_client_hdlr;
  void* context_;
};


#endif
