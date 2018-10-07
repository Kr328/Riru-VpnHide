#include "hook.h"

#include <stdio.h>

#define BINDER_INTERNAL_CLASS_PATH              "com/android/internal/os/BinderInternal"
#define BINDER_INTERNAL_GET_CONTEXT_OBJECT_NAME "getContextObject"

static jint (*originalRegisterNatives)(JNIEnv *env ,const char *class_name ,const JNINativeMethod* methods, jint length);
static jint hookedRegisterNatives(JNIEnv *env ,const char *class_name ,const JNINativeMethod* methods, jint length);

static jobject (*originalGetContextObject)(JNIEnv* env, jobject clazz);
static jobject hookedGetContextObject(JNIEnv* env, jobject clazz);

static jobject jniGetContextObjectOriginal(JNIEnv* env, jobject clazz) {
    return originalGetContextObject(env ,clazz);
}

static int gIsDexLoaded = 0;
static jmethodID javaHookedGetContextObjectId = NULL;
static jclass    javaHookedInjectClass        = NULL;

int hook_install(JNIEnv *env) {
	int (*register_android_os_Binder)(JNIEnv* env) = dlsym(dlopen(NULL ,RTLD_LAZY) ,"_Z26register_android_os_BinderP7_JNIEnv");
	if ( register_android_os_Binder == NULL )
		return -1;

	if ( originalRegisterNatives == NULL ) {
	    xhook_register(".*android_runtime.*" ,"jniRegisterNativeMethods" ,&hookedRegisterNatives ,(void **) &originalRegisterNatives);
	    if ( xhook_refresh(0) == 0 )
	        xhook_clear();
	    else
	        return -1;
	}
	
	register_android_os_Binder(env);

	return 0;
}

static const JNINativeMethod gBinderInternalMethod = {
     /* name, signature, funcPtr */
    BINDER_INTERNAL_GET_CONTEXT_OBJECT_NAME, "()Landroid/os/IBinder;", (void*)&hookedGetContextObject
};

static const JNINativeMethod gBinderInjectorMethod = {
     /* name, signature, funcPtr */
    INJECTOR_GET_CONTEXT_OBJECT_NAME,        "()Landroid/os/IBinder;", (void*)&jniGetContextObjectOriginal
};

static jint hookedRegisterNatives(JNIEnv *env ,const char *class_name ,const JNINativeMethod* methods, jint length) {
    int is_need_register = 0;

	if ( strcmp(BINDER_INTERNAL_CLASS_PATH ,class_name) == 0 ) {
		LOGI("Class com/android/internal/os/BinderInternal found.");
		for ( int i = 0 ; i < length ; i++ ) {
		    if ( strcmp(BINDER_INTERNAL_GET_CONTEXT_OBJECT_NAME ,methods[i].name) == 0 ) {
		        LOGI("Method getContextBinder found. %p" ,methods[i].fnPtr);
		        originalGetContextObject = methods[i].fnPtr;
		        is_need_register = 1;
		        break;
		    }
		}
	}
	
	jint result = originalRegisterNatives(env ,class_name ,methods ,length);

	if ( is_need_register )
	    !originalRegisterNatives(env ,class_name ,&gBinderInternalMethod ,1) && LOGI("Registered %s" ,BINDER_INTERNAL_GET_CONTEXT_OBJECT_NAME);

	return result;
}

static jobject hookedGetContextObject(JNIEnv* env, jobject clazz) {
    LOGI("getContextObject called.");

    if ( !gIsDexLoaded ) {
        if ( java_helper_load_dex_find_method(env ,DEX_PATH ,DEX_ODEX_PATH ,DEX_INJECT_CLASS_NAME ,DEX_INJECT_METHOD_NAME ,"()Landroid/os/IBinder;" ,&javaHookedInjectClass ,&javaHookedGetContextObjectId) )
            javaHookedGetContextObjectId = NULL;

        (*env)->RegisterNatives(env ,javaHookedInjectClass ,&gBinderInjectorMethod ,1);

        gIsDexLoaded = 1;
    }

    if ( javaHookedGetContextObjectId ) {
        jobject result = (*env)->CallStaticObjectMethod(env ,javaHookedInjectClass ,javaHookedGetContextObjectId);

        if ( (*env)->ExceptionCheck(env) ) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
            LOGE("Call method failure");
        }
        else
            return result;
    }

    return originalGetContextObject(env ,clazz);
}