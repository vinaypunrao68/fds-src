/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <StorHvisorCPP.h>
#include <util/fds_stat.h>
#include <native_api.h>
#include <fds_process.h>
#include "com_formationds_nativeapi_NativeApi.h"
// #include "BucketStats.h"

fds::FDS_NativeAPI *api;

JNIEXPORT void JNICALL Java_com_formationds_nativeapi_NativeApi_init
(JNIEnv *, jclass klass) {
    int argc = 1;
    char *argv[] = {"fds_java_backend"};
    fds::init_process_globals("am.log");
    api = new fds::FDS_NativeAPI(fds::FDS_NativeAPI::FDSN_AWS_S3);
    CreateSHMode(argc, argv, NULL, NULL, false, 0, 0);

    fds::Module *am_mod_vec[] = {
        &fds::gl_fds_stat,
        nullptr
    };
    fds::ModuleVector am_module(0, NULL, am_mod_vec);

    am_module.mod_execute();
}



class BucketStatsHandler {
  public:
    BucketStatsHandler(jclass javaHandlerClass, jobject javaHandler);
    
    static void callback(const std::string& timestamp,
                  int content_count,
                  const BucketStatsContent* contents,
                  void *req_context,
                  void *callbackData,
                  FDSN_Status status,
                  ErrorDetails *err_details);    
};


BucketStatsHandler::BucketStatsHandler(jclass javaHandlerClass, jobject javaHandler) {

}

void BucketStatsHandler::callback(const std::string& timestamp,
                                int content_count,
                                const BucketStatsContent* contents,
                                void *req_context,
                                void *callbackData,
                                FDSN_Status status,
                                ErrorDetails *err_details) {
    
}
 
JNIEXPORT void JNICALL Java_com_formationds_nativeapi_NativeApi_getBucketsStats 
(JNIEnv *env, jclass klass, jobject o) {
    BucketStatsHandler *handler = new BucketStatsHandler(klass, o);
    api->GetBucketStats(
        static_cast<void *>(NULL), 
        static_cast<fdsnBucketStatsHandler>(handler->callback), 
        static_cast<void *>(NULL));
}

int main(int argc, char *argv[]) {
    return 0;
}
