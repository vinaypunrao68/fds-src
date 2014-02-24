/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <jni.h>
#include <iostream>
#include "./JavaContext.h"

namespace fds {
    namespace java {
        JavaContext::JavaContext(JavaVM *javaVM, std::vector<jobject> args) {
            this->javaVM = javaVM;
            this->args = args;
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
        
        jobject JavaContext::invoke(JNIEnv *env, jobject o, char *methodName, char *signature, ...) {
            va_list args;
            va_start(args, signature);
            jclass klass = env->GetObjectClass(o);
            jmethodID method = env->GetMethodID(klass, methodName, signature);            
	    jobject result = env->CallObjectMethodV(o, method, args);
            va_end(args);
            return result;
        }

        jint JavaContext::invokeInt(JNIEnv *env, jobject o, char *methodName, char *signature, ...) {
            va_list args;
            va_start(args, signature);
            jclass klass = env->GetObjectClass(o);
            jmethodID method = env->GetMethodID(klass, methodName, signature);            
	    jint result = env->CallIntMethodV(o, method, args);
            va_end(args);
            return result;            
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

        std::string JavaContext::ccString(JNIEnv *env, jstring javaString) {
            const char *buf = env->GetStringUTFChars(javaString, NULL);
            std::string s = std::string(buf);
            env->ReleaseStringUTFChars(javaString, buf);
            return s;
        }

        JavaContext::~JavaContext() {
        }
    }
}
