/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <thread>
#include <iostream>
#include <jni.h>
#include <stdio.h>

#include "./JavaContext.h"
#include "./JavaMain.h"

namespace fds {
    void startOmJvm() {
        JavaVM *javaVM;
        JNIEnv* env;
        JavaVMInitArgs args;
        JavaVMOption options[5];
        JavaContext *javaContext;

        args.version = JNI_VERSION_1_8;
        args.nOptions = 2;
        options[0].optionString = "-Djava.class.path=../lib/java/classes/";
        options[1].optionString = "-Dfile.encoding=UTF-8";
        options[2].optionString = "-verbose:jni";
        options[3].optionString = "-Xdebug";
        options[4].optionString =
            "-Xrunjdwp:server=y,transport=dt_socket,address=4000,suspend=y";

        args.options = options;
        args.ignoreUnrecognized = JNI_FALSE;
        JNI_CreateJavaVM(&javaVM, reinterpret_cast<void **>(&env), &args);
        javaContext = new JavaContext(javaVM);
        env = javaContext->attachCurrentThread();
        jobject bootstrapper = javaContext->javaInstance(env,
                      "com/formationds/util/Bootstrapper");

        javaContext->invoke(env, bootstrapper, "start", "()V");

        std::cout << "Started JVM" << std::endl;
        // OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    }
}  // namespace fds
