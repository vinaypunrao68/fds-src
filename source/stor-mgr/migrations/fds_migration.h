#ifndef _FDS_MIGRATIONS_H
#define _FDS_MIGRATIONS_H

#include <string>
#include <fds_module.h>
#include <fdsp/FDSP_MigrationPathReq.h>
#include <fdsp/FDSP_MigrationPathResp.h>

namespace fds
{

/* Service for migrating objects */
class FdsMigrationSvc : public Module,
    protected FDSP_MigrationPathReqHandler, 
    protected FDSP_MigrationPathRespHandler  {

 public:
  FdsMigrationSvc();

  /* Overrides from Module */
  virtual void mod_startup() override ;
  virtual void mod_shutdown() override ;

 protected:

  /* RPC Overrides from FDSP_MigrationPathReqHandler  */
  void MigrateToken(const FDSP_MsgHdrType& fdsp_msg, 
                    const FDSP_MigrateTokenReq& migrate_req) override {
      // Don't do anything here. This stub is just to keep cpp compiler happy
  }
  void MigrateToken(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, 
                    boost::shared_ptr<FDSP_MigrateTokenReq>& migrate_req) override;
  void PutTokenObjects(const FDSP_MsgHdrType& fdsp_msg, 
                       const PutTokenObjectsReq& mig_put_req) override {
      // Don't do anything here. This stub is just to keep cpp compiler happy
  }
  void PutTokenObjects(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, 
                       boost::shared_ptr<PutTokenObjectsReq>& mig_put_req) override;

  /* RPC Overrides from FDSP_MigrationPathRespHandler  */
  void MigrateTokenResp(const FDSP_MsgHdrType& fdsp_msg) override {
      // Don't do anything here. This stub is just to keep cpp compiler happy
  }
  void MigrateTokenResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg) override;
  void PutTokenObjectsResp(const FDSP_MsgHdrType& fdsp_msg) override {
      // Don't do anything here. This stub is just to keep cpp compiler happy
  }
  void PutTokenObjectsResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg) override;
  
 private:

}; 
}  // namespace fds

#endif
