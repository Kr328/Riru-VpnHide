#pragma once

#include <jni.h>
#include <dlfcn.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "xhook.h"

#include "log.h"
#include "javahelper.h"

//#define DEX_PATH               "/data/local/tmp/injector.jar"
#define DEX_PATH               "/data/misc/riru/modules/vpnhide/injector.jar"
#define DEX_ODEX_PATH          "/data/dalvik-cache"
#define DEX_INJECT_CLASS_NAME  "me.kr328.vpnhide.Injector"
#define DEX_INJECT_METHOD_NAME "getContextObjectHooked"

#define INJECTOR_CLASS_PATH               "me/kr328/vpnhide/Injector"
#define INJECTOR_GET_CONTEXT_OBJECT_NAME  "getContextObjectOriginal"

int hook_install(JNIEnv *env);
