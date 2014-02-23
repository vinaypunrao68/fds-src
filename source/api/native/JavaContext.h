/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef _JAVA_CONTEXT_H
#define _JAVA_CONTEXT_H

namespace fds {
    namespace java {
        
        class JavaContext {
        public: 
            JavaVM *javaVM;
            jobject o;
            
            JavaContext(JavaVM *javaVM, jobject o);
            JNIEnv *attachCurrentThread();
            void detachCurrentThread();
            void invoke(JNIEnv *env, jobject o, char *methodName, char *signature, ...);
            jobject javaInstance(JNIEnv *env, char *className);
            jobject javaInstance(JNIEnv *env, char *className, char *constructorSignature, ...);
            jstring javaString(JNIEnv *env, std::string s);
            ~JavaContext();
        };
    }
}  

#endif
