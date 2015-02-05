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
    OMSvcProcess(int argc, char *argv[],
                   const std::string &def_cfg_file,
                   const std::string &base_path,
                   const std::string &def_log_file,
                   fds::Module **mod_vec);
    virtual ~OMSvcProcess();

    void init(int argc, char *argv[],
         const std::string &def_cfg_file,
         const std::string &base_path,
         const std::string &def_log_file,
         fds::Module **mod_vec);

    virtual void registerSvcProcess() override;

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
