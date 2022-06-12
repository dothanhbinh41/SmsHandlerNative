package com.SmsHandlerNative;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;


public class SmsListener extends BroadcastReceiver {
 	static { 
            System.loadLibrary("SmsHandler"); // Load native library hello.dll (Windows) or libhello.so (Unixes)                           
       }
    @Override
    public void onReceive(Context context, Intent intent) { 
            onReceived(intent);
    } 

    public native void onReceived(Intent intent); 
}