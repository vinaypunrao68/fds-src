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
        
        JavaContext::~JavaContext() {
        }
    }
}
