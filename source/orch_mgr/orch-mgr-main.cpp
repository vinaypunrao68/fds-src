/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>
#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <orch-mgr/om-service.h>
#include <thread>
#include <jni.h>
#include <stdio.h>
#include "./JavaContext.h"

namespace fds {

extern OrchMgr *orchMgr;
OM_Module       gl_OMModule("OM");

OM_Module *
OM_Module::om_singleton()
{
    return &gl_OMModule;
}

}  // namespace fds

void start_jvm() {
    JavaVM *javaVM;
    JNIEnv* env;
    JavaVMInitArgs args;
    JavaVMOption options[2];
    JavaContext *javaContext;

    args.version = JNI_VERSION_1_8;
    args.nOptions = 3;
    options[0].optionString = "-Djava.class.path=../lib/java/classes/";
    options[1].optionString = "-Dfile.encoding=UTF-8";
    options[2].optionString = "-verbose:jni";
    // options[3].optionString =
    //     "-Xdebug -Xrunjdwp:server=y, transport=dt_socket,address=4000, suspend=y";

    args.options = options;
    args.ignoreUnrecognized = JNI_FALSE;
    JNI_CreateJavaVM(&javaVM, reinterpret_cast<void **>(&env), &args);
    javaContext = new JavaContext(javaVM);
    env = javaContext->attachCurrentThread();
    jobject bootstrapper = javaContext->javaInstance(env,
                                                     "com/formationds/util/Bootstrapper");
    javaContext->invoke(env, bootstrapper, "start", "()V");
    fflush(stdout);
    cout << "Started JVM" << endl;
    // OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
}

int main(int argc, char *argv[])
{
    fds::Module *omVec[] = {
        &fds::gl_OMModule,
        NULL
    };
    fds::orchMgr = new fds::OrchMgr(argc, argv, "orch_mgr.conf", "fds.om.", omVec);
    fds::gl_orch_mgr = fds::orchMgr;

    fds::orchMgr->setup();
    std::thread vmThread(start_jvm);

    fds::orchMgr->run();
    delete fds::orchMgr;
    return 0;
}
