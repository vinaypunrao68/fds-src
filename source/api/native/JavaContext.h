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
            ~JavaContext();
        };
    }
}  

#endif
