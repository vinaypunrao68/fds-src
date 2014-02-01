#include <fds_migration.h>

namespace fds {

FdsMigrationSvc::FdsMigrationSvc() 
{
    // Your initialization goes here
}

void FdsMigrationSvc::MigrateToken(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, 
                  boost::shared_ptr<FDSP_MigrateTokenReq>& migrate_req) 
{
    objStorMgr->MigrateToken(fdsp_msg, migrate_req);
    /* TODO: */
    /* Make sure we don't have duplicate migrate requests */
    /* Send Message to StorMgr to iterate objects along with metadata */
}

void FdsMigrationSvc::iterate_objects_cb()
{
    /* TODO: */
    /* Populate the response message */
    /* Send the response */
}

void FdsMigrationSvc::PutTokenObjects(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg, 
                     boost::shared_ptr<PutTokenObjectsReq>& mig_put_req) 
{
    objStorMgr->PutTokenObjects(fdsp_msg, mig_put_req);
    /* TODO: */
    /* Construct a message to Put the objects */
    /* Send response to client */
}

FdsMigrationSvc::FDSP_MigrationPathRespHandler()
{
}

void FdsMigrationSvc::
MigrateTokenResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg)
{
    /* TODO: Handle errors */
}

void FdsMigrationSvc::
PutTokenObjectsResp(boost::shared_ptr<FDSP_MsgHdrType>& fdsp_msg)
{
    /* TODO: Handle any errors */
    /* Here source may tell us to throttle how much we are asking */
}

}  // namespace fds
