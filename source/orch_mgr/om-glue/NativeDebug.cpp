/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <stdio.h>

#include "./com_formationds_util_Bootstrapper.h"

JNIEXPORT
void JNICALL Java_com_formationds_util_Bootstrapper_printf(JNIEnv *env,
                                                          jclass c,
                                                          jstring s) {
    const char* cString = env->GetStringUTFChars(s, 0);
    printf("%s\n", cString);
    fflush(stdout);
    env->ReleaseStringUTFChars(s, cString);
}
