/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <unistd.h>
#include <fds_error.h>
#include <concurrency/fds_actor.h>
#include <concurrency/fds_actor_request.h>
#include <fds_assert.h>

namespace fds {
class FdsActorTest : public FdsRequestQueueActor {
public:
    FdsActorTest(fds_threadpoolPtr threadpool)
    : FdsRequestQueueActor("FdsActorTest", nullptr, threadpool)
    {
        copy_complete_cnt = 0;
        err_cnt = 0;
    }
    void copy_token(FdsActorRequestPtr req)
    {
        copy_complete_cnt++;
    }
    
    virtual Error handle_actor_request(FdsActorRequestPtr req) override
    {
        Error err(ERR_OK);
        switch (req->type) {
        case FAR_ID(MigSvcCopyTokensReq):
            threadpool_->schedule(&FdsActorTest::copy_token, this, req);
            break;
        default:
            err_cnt++;
            err = ERR_FAR_INVALID_REQUEST;
        }
        return err;
    }
    
    int copy_complete_cnt;
    int err_cnt = 0;
};

void test_send_actor_request()
{
    fds_threadpoolPtr threadpool(new fds_threadpool());
    FdsActorRequestPtr r(new FdsActorRequest());
    r->type = FAR_ID(MigSvcCopyTokensReq);

    FdsActorTest t(threadpool);
    t.send_actor_request(r);
    sleep(5);

    fds_verify(t.copy_complete_cnt == 1);
}
}

int main() {
    fds::test_send_actor_request();

    return 0;
}

