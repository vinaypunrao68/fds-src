/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <StorHvisorNet.h>
#include <am-engine/am-ubd-connect.h>
#include <am-engine/handlers/handlermappings.h>
#include <am-engine/handlers/responsehandler.h>

namespace fds {

AmUbdConnect::AmUbdConnect(const std::string &name)
        : Module(name.c_str()) {
}

AmUbdConnect::~AmUbdConnect() {
}

int
AmUbdConnect::mod_init(SysParams const *const param) {
    return 0;
}

void
AmUbdConnect::mod_startup() {
}

void
AmUbdConnect::mod_shutdown() {
}

void
AmUbdConnect::init_server(FDS_NativeAPI::ptr api) {
    am_api = api;
}

Error
AmUbdConnect::amUbdPutBlob(fbd_request_t *blkReq) {
    // Get the volume name from the UUID
    std::string volumeName =
            storHvisor->vol_table->getVolumeName(blkReq->volUUID);

    // Create bucket context
    // TODO(Andrew): we dont really need this...
    BucketContext bucket_ctx("host", volumeName, "accessid", "secretkey");

    // Create context handler
    // TODO(Andrew): Change to blk specific handler
    PutObjectBlkResponseHandler *putHandler =
            new PutObjectBlkResponseHandler(blkReq->cb_request,  // Callback function
                                            blkReq->vbd,  // Callback arg1
                                            blkReq->vReq,  // Callback arg2
                                            blkReq);  // Callback arg3

    // Create NULL putProps for now
    PutPropertiesPtr putProps;

    // Do async putobject
    // TODO(Andrew): The error path callback maybe called
    // in THIS thread's context...need to fix or handle that.
    am_api->PutObject(&bucket_ctx,
                      "blkdev0",  // TODO(Andrew): Don't hardcode
                      putProps,
                      NULL,  // Not passing any context for the callback
                      blkReq->buf,  // Data buf
                      blkReq->sec * 512,  // Offset; Don't hardcode sector size
                      blkReq->len,  // TODO(Andrew): Verify length?
                      false,  // Always make is last
                      fn_PutObjectBlkHandler,
                      static_cast<void *>(putHandler));

    return ERR_OK;
}

AmUbdConnect gl_AmUbdConnect("Global AmUbd connector");

}  // namespace fds

BEGIN_C_DECLS
int
amUbdPutBlob(fbd_request_t *blkReq) {
    Error err = fds::gl_AmUbdConnect.amUbdPutBlob(blkReq);

    if (err != ERR_OK) {
        return -1;
    }
    return 0;
}

int
amUbdGetBlob(fbd_request_t *blkReq) {
    return 0;
}
END_C_DECLS
