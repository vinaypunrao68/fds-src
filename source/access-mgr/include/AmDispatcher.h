/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_

#include <string>
#include <fds_module.h>
#include <StorHvVolumes.h>

namespace fds {

/**
 * AM FDSP request dispatcher and reciever. The dispatcher
 * does the work to send and receive AM network messages over
 * the service layer. The layer is stateless and does not
 * internal locking.
 */
class AmDispatcher : public Module, public boost::noncopyable {
  public:
    explicit AmDispatcher(const std::string &modName);
    ~AmDispatcher();
    typedef std::unique_ptr<AmDispatcher> unique_ptr;

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    /**
     * Issues a start blob transaction request.
     */
    void startBlobTx(AmQosReq *qosReq);

  private:
    /// Uturn test all network requests
    fds_bool_t uturnAll;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
