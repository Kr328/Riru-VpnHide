#include "javahelper.h"

#define TAG "RiruJavaHelper"

#define ADDRESS_ARRAY_LENGTH 12

int java_helper_load_dex_find_method(JNIEnv *env ,const char *dex_path ,const char *odex_path ,const char *clazz_name ,const char *method_name ,const char *signature ,jclass *clazz ,jmethodID *method) {
    // get system class loader
    jclass    classClassLoader           = (*env)->FindClass(env ,"java/lang/ClassLoader");
    jmethodID methodGetSystemClassLoader = (*env)->GetStaticMethodID(env ,classClassLoader, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    jobject   objectSystemClassLoader    = (*env)->CallStaticObjectMethod(env ,classClassLoader, methodGetSystemClassLoader);

    // load dex
    jstring   stringDexPath              = (*env)->NewStringUTF(env ,dex_path);
    jstring   stringOdexPath             = (*env)->NewStringUTF(env ,odex_path);
    jclass    classDexClassLoader        = (*env)->FindClass(env ,"dalvik/system/DexClassLoader");
    jmethodID methodDexClassLoaderInit   = (*env)->GetMethodID(env ,classDexClassLoader, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    jobject   objectDexClassLoader       = (*env)->NewObject(env ,classDexClassLoader, methodDexClassLoaderInit, stringDexPath, stringOdexPath, NULL, objectSystemClassLoader);

    // get loaded dex inject method
    jmethodID methodFindClass            = (*env)->GetMethodID(env ,classDexClassLoader, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring   stringInjectClassName      = (*env)->NewStringUTF(env ,clazz_name);
    *clazz                               = (jclass)(*env)->CallObjectMethod(env ,objectDexClassLoader, methodFindClass, stringInjectClassName);

    // find method
    *method                              = (*env)->GetStaticMethodID(env ,*clazz, method_name, signature);

    // check status
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        __android_log_print(ANDROID_LOG_ERROR ,TAG ,"Inject dex failure");
        return -1;
    }

    return 0;
}
