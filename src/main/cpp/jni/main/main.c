#include <stdio.h>
#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>

#include "hook.h"

#include "log.h"

__attribute__((visibility("default"))) int nativeForkAndSpecializePost(JNIEnv *env, jclass clazz, jint res);

int nativeForkAndSpecializePost(JNIEnv *env, jclass clazz, jint res) {
    return res == 0 ? hook_install(env) : 0;
}
