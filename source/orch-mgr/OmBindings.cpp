/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <stdio.h>
#include <orchMgr.h>
#include <string>
#include <OmResources.h>

#include "./com_formationds_om_NativeOm.h"


JavaVM *javaVM;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* aReserved)
{
    printf("JNI loaded OK\n");
    fflush(stdout);

    javaVM = vm;
    return JNI_VERSION_1_8;
}

extern int main(int argc, const char *argv[]);

JNIEXPORT void JNICALL Java_com_formationds_om_NativeOm_init
(JNIEnv *env, jclass klass, jobjectArray javaArgs) {
    int length = env->GetArrayLength(javaArgs);
    const char *args[length + 1];
    args[0] = "orchMgr";
    for (int i = 0; i <length; i++) {
        jstring arg = (jstring)env->GetObjectArrayElement(javaArgs, i);
        const char *buf = env->GetStringUTFChars(arg, NULL);
        std::string *s = new std::string(buf);
        env->ReleaseStringUTFChars(arg, buf);
        args[i + 1] = s->c_str();
    }

    printf("JNI init done\n");
    main(length + 1, args);
}
