
package com.SmsHandlerNative;

import android.app.Activity;
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

        /* Create a TextView and set its text to "Hello world" */
        TextView  tv = new TextView(this);
        tv.setText("Hello World!");
        setContentView(tv);
    }
}
