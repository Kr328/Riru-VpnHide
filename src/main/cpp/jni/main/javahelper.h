#pragma once

#include <jni.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <android/log.h>

int java_helper_load_dex_find_method(JNIEnv *env ,const char *dex_path ,const char *odex_path ,const char *clazz_name ,const char *method_name ,const char *signature ,jclass *clazz ,jmethodID *method);

