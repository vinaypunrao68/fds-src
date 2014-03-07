/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <stdio.h>
#include <orchMgr.h>
#include <string>
#include <OmResources.h>

#include "./com_formationds_om_NativeApi.h"
#include "./JavaContext.h"


JavaVM *javaVM;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* aReserved)
{
    printf("JNI loaded OK\n");
    fflush(stdout);

    javaVM = vm;
    return JNI_VERSION_1_8;

    // this->arg = static_cast<jobject>(env->NewGlobalRef(arg));
}

extern int main(int argc, const char *argv[]);

JNIEXPORT void JNICALL Java_com_formationds_om_NativeApi_init
(JNIEnv *env, jclass klass, jobjectArray javaArgs) {
    JavaContext javaContext(javaVM);
    int length = env->GetArrayLength(javaArgs);
    const char *args[length + 1];
    args[0] = "orchMgr";
    for (int i = 0; i <length; i++) {
        jstring arg = (jstring)env->GetObjectArrayElement(javaArgs, i);
        std::string *cString = javaContext.ccString(env, arg);
        args[i + 1] = cString->c_str();
    }

    printf("JNI init done\n");
    main(length + 1, args);
}

static void acceptNode(jobject acceptor,
                       const char *nodeType,
                       NodeAgent::pointer peer) {
    JavaContext javaContext(javaVM);
    JNIEnv *env = javaContext.attachCurrentThread();
    jstring nodeName = javaContext.javaString(env, peer->get_node_name());
    javaContext.invoke(env, acceptor, "accept", "(Ljava/lang/Object;)V", nodeName);
    printf("Accepting one node\n");
    fflush(stdout);
}

JNIEXPORT void JNICALL Java_com_formationds_om_NativeApi_listNodes
(JNIEnv *env, jclass klass, jobject acceptor) {
    OM_NodeContainer *nodeContainer = OM_NodeDomainMod::om_loc_domain_ctrl();

    SmContainer::pointer smNodes = nodeContainer->dc_get_sm_nodes();
    smNodes->agent_foreach<jobject>(acceptor, "sm", acceptNode);

    JavaContext javaContext(javaVM);
    javaContext.invoke(env, acceptor, "finish", "()V");
}
