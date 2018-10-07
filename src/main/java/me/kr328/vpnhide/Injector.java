package me.kr328.vpnhide;

import android.app.ContentProviderHolder;
import android.app.DownloadManager;
import android.app.IActivityManager;
import android.app.IApplicationThread;
import android.app.job.JobParameters;
import android.content.ContentValues;
import android.content.Context;
import android.content.IContentProvider;
import android.net.*;
import android.os.IBinder;
import android.os.IServiceManager;
import android.os.RemoteException;
import android.os.ServiceManagerNative;
import android.provider.Downloads;
import android.util.Log;

import java.lang.reflect.Method;

@SuppressWarnings("unused")
public class Injector {
    public static IBinder getContextObjectHooked() {
        Log.i(Global.TAG ,"Java getContextObject called");

        return LocalInterfaceProxy.createInterfaceProxyBinder(ServiceManagerNative.asInterface(getContextObjectOriginal()) ,IServiceManager.class.getName() ,(original , replaced , method , args) -> {
            if ( "getService".equals(method.getName()) ) {
                switch ( args[0].toString() ) {
                    case Context.CONNECTIVITY_SERVICE :
                        return LocalInterfaceProxy.createInterfaceProxyBinder(IConnectivityManager.Stub.asInterface(original.getService(Context.CONNECTIVITY_SERVICE)) ,IConnectivityManager.class.getName() ,Injector::onConnectivityServiceCalled);
                    case Context.ACTIVITY_SERVICE :
                        return LocalInterfaceProxy.createInterfaceProxyBinder(IActivityManager.Stub.asInterface(original.getService(Context.ACTIVITY_SERVICE)) ,IActivityManager.class.getName() ,Injector::onActivityServiceCalled);
                }
            }
            return method.invoke(original ,args);
        });
    }

    private static Object onConnectivityServiceCalled(IConnectivityManager original , IConnectivityManager replaced , Method method ,Object[] args) throws Throwable {
        switch ( method.getName() ) {
            case "getNetworkCapabilities" :
                return logResult("getNetworkCapabilities" ,getNetworkCapabilities(original , (Network) args[0]));
            case "isActiveNetworkMetered" :
                return logResult("isActiveNetworkMetered" ,isActiveNetworkMetered(original));
            case "getNetworkInfoForUid" :
                return logResult("getNetworkInfoForUid" ,getNetworkInfoForUid(original ,(Network) args[0] ,(int)args[1] ,(boolean)args[2]));
            case "getNetworkInfo" :
                return logResult("getNetworkInfo" ,getNetworkInfo(original ,(int)args[0]));
            case "isActiveNetworkMeteredForUid" :
                return logResult("isActiveNetworkMeteredForUid" ,isActiveNetworkMetered(original));
        }

        return method.invoke(original ,args);
    }

    private static Object onActivityServiceCalled(IActivityManager original ,IActivityManager replaced ,Method method ,Object[] args) throws Throwable {
        switch ( method.getName() ) {
            case "getContentProvider" :
                return getContentProvider(original ,(IApplicationThread) args[0] ,(String) args[1] ,(int) args[2] ,(boolean) args[3]);
        }

        return method.invoke(original ,args);
    }

    private static NetworkCapabilities getNetworkCapabilities(IConnectivityManager original ,Network network) throws RemoteException {
        //Log.i(Global.TAG ,"Hooked getNetworkCapabilities called");

        NetworkCapabilities result = original.getNetworkCapabilities(network);

        if ( result != null && result.hasTransport(NetworkCapabilities.TRANSPORT_VPN) )
            return original.getNetworkCapabilities(findUnderlyingNetwork(original));
        return result;
    }

    private static boolean isActiveNetworkMetered(IConnectivityManager original) throws RemoteException {
        //Log.i(Global.TAG ,"Hooked isActiveNetworkMetered called");

        Network network = findUnderlyingNetwork(original);
        NetworkCapabilities networkCapabilities = original.getNetworkCapabilities(network);

        return !networkCapabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED);
    }

    private static NetworkInfo getNetworkInfoForUid(IConnectivityManager original ,Network network, int uid, boolean ignoreBlocked) throws RemoteException {
        //Log.i(Global.TAG ,"Hooked getNetworkInfoForUid called");

        NetworkInfo result = original.getNetworkInfoForUid(network ,uid ,ignoreBlocked);
        if ( result != null && result.getType() == ConnectivityManager.TYPE_VPN )
            return original.getActiveNetworkInfo();
        return result;
    }

    private static NetworkInfo getNetworkInfo(IConnectivityManager original ,int networkType) throws RemoteException {
        //Log.i(Global.TAG ,"Hooked getNetworkInfo called");

        if ( networkType == ConnectivityManager.TYPE_VPN )
            return original.getActiveNetworkInfo();
        return original.getNetworkInfo(networkType);
    }

    private static Network getActiveNetworkForUid(IConnectivityManager original ,int userId, boolean ignoreBlocked) throws RemoteException {
        //Log.i(Global.TAG ,"Hooked getActiveNetworkForUid called");

        Network result = original.getActiveNetworkForUid(userId ,ignoreBlocked);
        NetworkCapabilities capabilities = original.getNetworkCapabilities(result);

        if ( capabilities != null && capabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN) )
            return findUnderlyingNetwork(original);
        return result;
    }

    private static ContentProviderHolder getContentProvider(IActivityManager original ,IApplicationThread applicationThread, String auth, int userId, boolean stable) throws RemoteException {
        //Log.i(Global.TAG ,"Hooked getContentProvider called auth = " + auth);

        if ( "downloads".equals(auth) ) {
            ContentProviderHolder holder = original.getContentProvider(applicationThread ,auth ,userId ,stable);

            Log.i(Global.TAG ,"DownloadManager ContentProviderHolder found. provider = " + holder.provider );
            if ( holder.provider != null ) {
                holder.provider = ContentProviderProxy.createContentProviderProxy(holder.provider ,(orig ,replaced ,method ,args) -> {
                    if ( method.getName().equals("insert") && Downloads.Impl.CONTENT_URI.equals(args[1]) ) {
                        Log.i(Global.TAG ,"Modifying download request " + args[2]);

                        ContentValues contentValues = (ContentValues) args[2];

                        //Object isAllowMetered = contentValues.get(Downloads.Impl.COLUMN_ALLOW_METERED);
                        //if ( isAllowMetered != null ) {
                        //    if ( (boolean) isAllowMetered )
                        //        contentValues.put(Downloads.Impl.COLUMN_ALLOWED_NETWORK_TYPES , DownloadManager.Request.NETWORK_WIFI | DownloadManager.Request.NETWORK_MOBILE);
                        //    else
                        //        contentValues.put(Downloads.Impl.COLUMN_ALLOWED_NETWORK_TYPES , DownloadManager.Request.NETWORK_WIFI | DownloadManager.Request.NETWORK_MOBILE);
                        //}

                        contentValues.put(Downloads.Impl.COLUMN_ALLOWED_NETWORK_TYPES , DownloadManager.Request.NETWORK_WIFI | DownloadManager.Request.NETWORK_MOBILE);
                        contentValues.put(Downloads.Impl.COLUMN_ALLOW_METERED ,true);

                        Log.i(Global.TAG ,"Modified download request " + args[2]);
                    }

                    return method.invoke(orig ,args);
                });
            }

            return holder;
        }

        return original.getContentProvider(applicationThread ,auth ,userId ,stable);
    }

    private static Network findUnderlyingNetwork(IConnectivityManager connectivityManager) throws RemoteException {
        Network[]   networks          = connectivityManager.getAllNetworks();
        NetworkInfo defaultInfo       = connectivityManager.getActiveNetworkInfo();

        for ( Network network : networks ) {
            NetworkInfo currentInfo = connectivityManager.getNetworkInfoForUid(network ,android.os.Process.myUid() ,false);
            if ( defaultInfo != null && defaultInfo.toString().equals(currentInfo.toString()) )
                return network;
        }

        return connectivityManager.getActiveNetwork();
    }

    private static Object logResult(String method ,Object in) {
        //Log.i(Global.TAG ,"Result = " + in);
        return in;
    }

    public static native IBinder getContextObjectOriginal();
}
