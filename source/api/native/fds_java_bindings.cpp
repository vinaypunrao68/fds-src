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

#define BUFSIZE 4096

namespace fds {
    namespace java {
	fds::FDS_NativeAPI *api;
	JavaVM *javaVM;

        static void debug_buf(const char *msg, const char *buf, int size) {
            printf("%s: [", msg);
            for (int i = 0; i < size; i++) {
                if (i != 0) {
                    printf(" ");
                }
                printf("%d", buf[i]);
            }
            printf("]\n");
            fflush(stdout);
        }

        jobject javaBucketStats(JavaContext *javaContext, JNIEnv *env, int count, const BucketStatsContent *bucketStats) {
            jobject javaBuckets = javaContext->javaInstance(env, "java/util/ArrayList");

            for (int i = 0; i < count; i++) {
                BucketStatsContent b = bucketStats[i];
                jobject bucket = javaContext->javaInstance(env,
                                                           "com/formationds/nativeapi/BucketStatsContent",
                                                           "(Ljava/lang/String;IDDD)V",
                                                           javaContext->javaString(env, b.bucket_name),
                                                           (jint) b.priority,
                                                           (jdouble) b.performance,
                                                           (jdouble) b.sla,
                                                           (jdouble) b.limit);
                javaContext->invoke(env, javaBuckets, "add", "(Ljava/lang/Object;)Z", bucket);
            }

            return javaBuckets;
        }

	static void bucket_stats_cb(const std::string& timestamp,
                                    int content_count,
                                    const BucketStatsContent* contents,
                                    void *req_context,
                                    void *callback_data,
                                    FDSN_Status status,
                                    ErrorDetails *err_details) {
	    JavaContext *javaContext = static_cast<JavaContext *>(callback_data);
	    JNIEnv *env = javaContext->attachCurrentThread();
            jobject buckets = javaBucketStats(javaContext, env, content_count, contents);
            javaContext->invoke(env, javaContext->arg, "accept", "(Ljava/lang/Object;)V", buckets);
	    javaContext->detachCurrentThread();
	}

        static void status_cb(FDSN_Status status,
                              const ErrorDetails *errorDetails,
                              void *callbackData) {
	    JavaContext *javaContext = static_cast<JavaContext *>(callbackData);
	    JNIEnv *env = javaContext->attachCurrentThread();
            jobject javaStatus = javaContext->javaInstance(env, "java/lang/Integer", "(I)V", (jint) status);
            javaContext->invoke(env, javaContext->arg, "accept", "(Ljava/lang/Object;)V", javaStatus);
            javaContext->detachCurrentThread();
        }

        static int put_cb(void *reqContext,
                          fds_uint64_t bufferSize,
                          char *buffer,
                          void *callbackData,
                          FDSN_Status status,
                          ErrorDetails* errDetails) {
            debug_buf("put callback", buffer, (int)bufferSize);
	    JavaContext *javaContext = static_cast<JavaContext *>(callbackData);
	    JNIEnv *env = javaContext->attachCurrentThread();
            jobject javaStatus = javaContext->javaInstance(env, "java/lang/Integer", "(I)V", (jint) status);
            javaContext->invoke(env, javaContext->arg, "accept", "(Ljava/lang/Object;)V", javaStatus);
            javaContext->detachCurrentThread();
            return 0;
        }

        static FDSN_Status get_cb(void *reqContext,
                                  fds_uint64_t bufferSize,
                                  const char *buffer,
                                  void *callbackData,
                                  FDSN_Status status,
                                  ErrorDetails *errDetails) {
            debug_buf("get callback", buffer, (int)bufferSize);
	    JavaContext *javaContext = static_cast<JavaContext *>(callbackData);
	    JNIEnv *env = javaContext->attachCurrentThread();
            jobject javaStatus = javaContext->javaInstance(env, "java/lang/Integer", "(I)V", (jint) status);
            javaContext->invoke(env, javaContext->arg, "accept", "(Ljava/lang/Object;)V", javaStatus);
            javaContext->detachCurrentThread();
            return status;
        }

        static BucketContext *makeBucketContext(JNIEnv *env, JavaContext *javaContext, jstring bucketName) {
            return new BucketContext("host", javaContext->ccString(env, bucketName), "", "");
        }
    }
}

using namespace fds::java;

// JNI code needs to be in the global namespace
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
(JNIEnv *env, jclass klass, jobject javaConsumer) {
    JavaContext *javaContext = new JavaContext(javaVM, env, javaConsumer);

    api->GetBucketStats(
        static_cast<void *>(NULL),
        static_cast<fdsnBucketStatsHandler>(&bucket_stats_cb),
        static_cast<void *>(javaContext));
}

JNIEXPORT void JNICALL Java_com_formationds_nativeapi_NativeApi_createBucket
(JNIEnv *env, jclass klass, jstring bucketName, jobject javaConsumer) {
    JavaContext *javaContext = new JavaContext(javaVM, env, javaConsumer);
    BucketContext *bucketContext = makeBucketContext(env, javaContext, bucketName);

    api->CreateBucket(
        bucketContext,
        CannedAclPrivate,
        static_cast<void *>(NULL),
        static_cast<fdsnResponseHandler>(&status_cb),
        static_cast<void *>(javaContext));
}

JNIEXPORT void JNICALL Java_com_formationds_nativeapi_NativeApi_put
(JNIEnv *env, jclass klass, jstring bucketName, jstring objectName, jbyteArray bytes, jobject javaCallback) {
    JavaContext *javaContext = new JavaContext(javaVM, env, javaCallback);
    BucketContext *bucketContext = makeBucketContext(env, javaContext, bucketName);
    PutProperties *props = new PutProperties();
    char *buf = (char *)env->GetByteArrayElements(bytes, JNI_FALSE);
    int length = (int)env->GetArrayLength(bytes);
    debug_buf("before put", buf, length);
    api->PutObject(bucketContext,
                   javaContext->ccString(env, objectName),
                   props,
                   (void *)NULL,
                   buf,
                   length,
                   static_cast<fdsnPutObjectHandler>(&put_cb),
                   static_cast<void *>(javaContext));

}

JNIEXPORT void JNICALL Java_com_formationds_nativeapi_NativeApi_get
(JNIEnv *env, jclass klass, jstring bucketName, jstring objectName, jbyteArray bytes, jobject javaCallback) {
    JavaContext *javaContext = new JavaContext(javaVM, env, javaCallback);
    BucketContext *bucketContext = makeBucketContext(env, javaContext, bucketName);
    GetConditions *getConditions = new GetConditions();

    char *buf = (char *)env->GetByteArrayElements(bytes, JNI_FALSE);
    int length = (int)env->GetArrayLength(bytes);

    debug_buf("before get", buf, length);
    api->GetObject(bucketContext,
                   javaContext->ccString(env, objectName),
                   getConditions,
                   0,
                   length,
                   buf,
                   length,
                   static_cast<void *>(NULL),
                   static_cast<fdsnGetObjectHandler>(&get_cb),
                   static_cast<void *>(javaContext)
        );
}


int main(int argc, char *argv[]) {

    Java_com_formationds_nativeapi_NativeApi_init(NULL, NULL);

    JNIEnv* env;
    JavaVMInitArgs args;
    JavaVMOption options[2];

    args.version = JNI_VERSION_1_8;
    args.nOptions = 2;
    options[0].optionString = "-Djava.class.path=/home/fabrice/fds-src/source/api/target/classes/";
    options[1].optionString = "-verbose:jni";
    args.options = options;
    args.ignoreUnrecognized = JNI_FALSE;

    JNI_CreateJavaVM(&javaVM, (void **)&env, &args);

    jclass cls = env->FindClass("com/formationds/nativeapi/Acceptor");
    jmethodID constructor = env->GetMethodID(cls, "<init>", "()V");
    jobject consumer = env->NewObject(cls, constructor);

    Java_com_formationds_nativeapi_NativeApi_createBucket(
        env,
        cls,
        env->NewStringUTF("slimebucket"),
        consumer);

    sleep(10);


    if (javaVM->AttachCurrentThread((void **)(&env), NULL) != 0) {
        printf("Failed to attach current thread!\n");
        fflush(stdout);
    }

    Java_com_formationds_nativeapi_NativeApi_put(env,
                                                 cls,
                                                 env->NewStringUTF("slimebucket"),
                                                 env->NewStringUTF("thebytes"),
                                                 env->NewByteArray(64),
                                                 consumer);
    printf("Fired API call\n");
    sleep(10);

    Java_com_formationds_nativeapi_NativeApi_get(env,
                                                 cls,
                                                 env->NewStringUTF("slimebucket"),
                                                 env->NewStringUTF("thebytes"),
                                                 env->NewByteArray(64),
                                                 consumer);
    printf("Fired API call\n");
    sleep(10);

    return 0;
}
