
package com.SmsHandlerNative;

import android.app.Activity;
import android.app.NativeActivity;
import android.content.Intent;
import android.widget.TextView;
import android.os.Bundle;



public class SmsHandlerNative extends Activity
{
	static { 
            System.loadLibrary("SmsHandler"); // Load native library hello.dll (Windows) or libhello.so (Unixes)                           
       }
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState); 
        Intent myIntent = new Intent(SmsHandlerNative.this, NativeActivity.class);
        this.startActivity(myIntent);
    }
}
