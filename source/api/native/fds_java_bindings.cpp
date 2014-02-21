/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <StorHvisorCPP.h>
#include <util/fds_stat.h>
#include <native_api.h>
#include <fds_process.h>
#include <stdio.h>

#include "com_formationds_nativeapi_NativeApi.h"
#include "JavaContext.h"

namespace fds {
    namespace java {
	fds::FDS_NativeAPI *api;
	JavaVM *javaVM;
	
	static void
	bucket_stats_cb(const std::string& timestamp,
			int content_count,
			const BucketStatsContent* contents,
			void *req_context,
			void *callback_data,
			FDSN_Status status,
			ErrorDetails *err_details) {   
	    JavaContext *javaContext = static_cast<JavaContext *>(callback_data);    
	    JNIEnv *env = javaContext->attachCurrentThread();
	    
	    jclass klass = env->GetObjectClass(javaContext->o);
	    jmethodID method = env->GetMethodID(klass, "handle", "()V");
	    env->CallObjectMethod(javaContext->o, method);
	    
	    javaContext->detachCurrentThread();
	}
    }
}

using namespace fds::java;

// The JNI code needs to be in the global namespace
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* aReserved)
{
    javaVM = vm;
    return JNI_VERSION_1_8;
}

JNIEXPORT void JNICALL Java_com_formationds_nativeapi_NativeApi_init
(JNIEnv *, jclass klass) {
    int argc = 1;
    char *argv[] = { (char *)"fds_java_bindings" };

    // Lifted from am-app
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
 

JNIEXPORT void JNICALL Java_com_formationds_nativeapi_NativeApi_getBucketsStats 
(JNIEnv *env, jclass klass, jobject o) {
    JavaContext *javaContext = new JavaContext(javaVM, o);

    api->GetBucketStats(
        static_cast<void *>(NULL), 
        static_cast<fdsnBucketStatsHandler>(&bucket_stats_cb), 
        static_cast<void *>(javaContext));
}

int main(int argc, char *argv[]) {
    /*
    Java_com_formationds_nativeapi_NativeApi_init(NULL, NULL);
    
    JNIEnv* env;
    JavaVMInitArgs args;
    JavaVMOption options[2];
    
    args.version = JNI_VERSION_1_2;
    args.nOptions = 2;
    options[0].optionString = "-Djava.class.path=/home/fabrice/fds-src/source/api/target/classes/";
    options[1].optionString = "-verbose:jni";
    args.options = options;
    args.ignoreUnrecognized = JNI_FALSE;
    
    JNI_CreateJavaVM(&javaVM, (void **)&env, &args);
    
    jclass cls = env->FindClass("com/formationds/nativeapi/SampleHandler");
    jmethodID constructor = env->GetMethodID(cls, "<init>", "()V");
    jobject toyHandler = env->NewObject(cls, constructor);
    Java_com_formationds_nativeapi_NativeApi_getBucketsStats(env, cls, toyHandler);
    printf("Fired API call\n");
    sleep(2000);
    */
    return 0;
}
