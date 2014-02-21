/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <jni.h>
#include <iostream>
#include "./JavaContext.h"

namespace fds {
    namespace java {
        JavaContext::JavaContext(JavaVM *javaVM, jobject o) {
            this->javaVM = javaVM;
            this->o = o;
        }
        
        JNIEnv *JavaContext::attachCurrentThread() {
            JNIEnv *env;
            if (this->javaVM->AttachCurrentThread((void **)(&env), NULL) != 0) {
                std::cout << "Failed to attach" << std::endl;
            }
            return env;
        }

        void JavaContext::detachCurrentThread() {
            this->javaVM->DetachCurrentThread();
        }
        
        void JavaContext::invoke(JNIEnv *env, jobject o, char *methodName, char *signature, ...) {
            va_list args;
            va_start(args, signature);
            jclass klass = env->GetObjectClass(o);
            jmethodID method = env->GetMethodID(klass, methodName, signature);            
	    env->CallObjectMethodV(o, method, args);
            va_end(args);
        }

        jobject JavaContext::javaInstance(JNIEnv *env, char *className) {
            return javaInstance(env, className, "()V");
        }
        
        jobject JavaContext::javaInstance(JNIEnv *env, char *className, char *constructorSignature, ...) {
            va_list args;
            va_start(args, constructorSignature);
            jclass klass = env->FindClass(className);
            jmethodID constructor = env->GetMethodID(klass, "<init>", constructorSignature);
            jobject o = env->NewObjectV(klass, constructor, args);
            va_end(args);
            return o;
        }

        jstring JavaContext::javaString(JNIEnv *env, std::string s) {
            return env->NewStringUTF(s.c_str());
        }

        JavaContext::~JavaContext() {
        }
    }
}
