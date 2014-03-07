/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ORCH_MGR_JAVACONTEXT_H_
#define SOURCE_ORCH_MGR_JAVACONTEXT_H_

#include <vector>
#include <string>

namespace fds {
    class JavaContext {
    private:
        JavaVM *javaVM;

    public:
        explicit JavaContext(JavaVM *javaVM);

        JNIEnv *attachCurrentThread();

        void detachCurrentThread();

        jobject invoke(JNIEnv *env,
                       jobject o,
                       char *methodName,
                       char *signature, ...);

        jint invokeInt(JNIEnv *env,
                       jobject o,
                       char *methodName,
                       char *signature, ...);

        jobject javaInstance(JNIEnv *env,
                             char *className);

        jobject javaInstance(JNIEnv *env,
                             char *className,
                             char *constructorSignature, ...);

        jstring javaString(JNIEnv *env, std::string s);

        std::string *ccString(JNIEnv *env, jstring javaString);

        ~JavaContext();
    };
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_JAVACONTEXT_H_
