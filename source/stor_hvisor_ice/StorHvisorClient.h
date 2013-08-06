#ifndef __StorHvisorClient_h__
#define __StorHvisorClient_h__
#include <Ice/ProxyF.h>
#include <Ice/ObjectF.h>
#include <Ice/Exception.h>
#include <Ice/LocalObject.h>
#include <Ice/StreamHelpers.h>
#include <Ice/Proxy.h>
#include <Ice/Object.h>
#include <Ice/Outgoing.h>
#include <Ice/OutgoingAsync.h>
#include <Ice/Incoming.h>
#include <Ice/IncomingAsync.h>
#include <Ice/Direct.h>
#include <Ice/FactoryTableInit.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Optional.h>
#include <Ice/StreamF.h>
#include <Ice/UndefSysMacros.h>
#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FDSP.h>

#ifndef ICE_IGNORE_VERSION
#   if ICE_INT_VERSION / 100 != 305
#       error Ice version mismatch!
#   endif
#   if ICE_INT_VERSION % 100 > 50
#       error Beta header file detected
#   endif
#   if ICE_INT_VERSION % 100 < 0
#       error Ice patch level mismatch!
#   endif
#endif

using namespace FDS_ProtocolInterface;

class StorHvisorClient : public Ice::Application
{
public:
    StorHvisorClient();
    int run(int, char*[]);

private:
    void exception(const Ice::Exception&);
};


class SHvisorClientCallback : public IceUtil::Shared
{
public:

    void response()
    {
    }

    void exception(const Ice::Exception& ex)
    {
        std::cerr << " AMI call to ObjStorMgr failed:\n" << ex << std::endl;
    }
};

typedef IceUtil::Handle<SHvisorClientCallback> shClientCallbackPtr;

#endif
