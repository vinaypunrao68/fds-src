#ifndef __StorHvisorClient_h__
#define __StorHvisorClient_h__


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
