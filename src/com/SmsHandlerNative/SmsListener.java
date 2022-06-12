package com.SmsHandlerNative;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;


public class SmsListener extends BroadcastReceiver {
 
    @Override
    public void onReceive(Context context, Intent intent) { 
            onReceived(intent);
    } 

    public native void onReceived(Intent intent); 
}