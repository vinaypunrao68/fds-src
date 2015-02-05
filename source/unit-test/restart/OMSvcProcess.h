/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_OMSVCPROCESS_H_
#define SOURCE_INCLUDE_NET_OMSVCPROCESS_H_

#include <boost/shared_ptr.hpp>

/* Forward declarations */
namespace FDS_ProtocolInterface {
}  // namespace FDS_ProtocolInterface
namespace fpi = FDS_ProtocolInterface;

namespace fds {

/* Forward declarations */
struct SvcProcess;

/**
* @brief OMSvcProcess
*/
struct OMSvcProcess : SvcProcess {
    OMSvcProcess(int argc, char *argv[]);
    virtual ~OMSvcProcess();

    void init(int argc, char *argv[]);

    virtual void registerSvcProcess() override;

    virtual int run() override;

 protected:
    /**
    * @brief Sets up configdb used for persistence
    */
    virtual void setupConfigDb_() override;

    /**
    * @brief Populates service information
    */
    virtual void setupSvcInfo_() override;

};
}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_OMSVCPROCESS_H_
