/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef _JAVA_CONTEXT_H
#define _JAVA_CONTEXT_H

#include <vector>

namespace fds {
    namespace java {
        
        class JavaContext {
        private:
            jclass klass;

        public: 
            JavaVM *javaVM;
            jobject arg;

            JavaContext(JavaVM *javaVM, JNIEnv *env, jobject arg);
            JNIEnv *attachCurrentThread();
            void detachCurrentThread();
            jobject invoke(JNIEnv *env, jobject o, char *methodName, char *signature, ...);
            jint invokeInt(JNIEnv *env, jobject o, char *methodName, char *signature, ...);
            jobject javaInstance(JNIEnv *env, char *className);
            jobject javaInstance(JNIEnv *env, char *className, char *constructorSignature, ...);
            jstring javaString(JNIEnv *env, std::string s);
            std::string ccString(JNIEnv *env, jstring javaString);
            ~JavaContext();
        };
    }
}  

#endif
