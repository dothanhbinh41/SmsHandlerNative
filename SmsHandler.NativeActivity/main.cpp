/*
* Copyright (C) 2010 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <malloc.h>
#include "permissions.cpp"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

/**
* Our saved state data.
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
};

/**
* Shared state for our app.
*/
struct engine {
	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	struct saved_state state;
};

/**
* Initialize an EGL context for the current display.
*/
static int engine_init_display(struct engine* engine) {
	// initialize OpenGL ES and EGL

	/*
	* Here specify the attributes of the desired configuration.
	* Below, we select an EGLConfig with at least 8 bits per color
	* component compatible with on-screen windows
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* Here, the application chooses the configuration it desires. In this
	* sample, we have a very simplified selection process, where we pick
	* the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	* guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	* As soon as we picked a EGLConfig, we can safely reconfigure the
	* ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;

	// Initialize GL state.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

	return 0;
}

/**
* Just the current frame in the display.
*/
static void engine_draw_frame(struct engine* engine) {
	if (engine->display == NULL) {
		// No display.
		return;
	}

	// Just fill the screen with a color.
	glClearColor(((float)engine->state.x) / engine->width, engine->state.angle,
		((float)engine->state.y) / engine->height, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(engine->display, engine->surface);
}

/**
* Tear down the EGL context currently associated with the display.
*/
static void engine_term_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

/**
* Process the next input event.
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		engine->state.x = AMotionEvent_getX(event, 0);
		engine->state.y = AMotionEvent_getY(event, 0);
		return 1;
	}
	return 0;
}

/**
* Process the next main command.
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// The system has asked us to save our current state.  Do so.
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// The window is being shown, get it ready.
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// When our app gains focus, we start monitoring the accelerometer.
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_enableSensor(engine->sensorEventQueue,
				engine->accelerometerSensor);
			// We'd like to get 60 events per second (in microseconds).
			ASensorEventQueue_setEventRate(engine->sensorEventQueue,
				engine->accelerometerSensor, (1000L / 60) * 1000);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		// When our app loses focus, we stop monitoring the accelerometer.
		// This is to avoid consuming battery while not being used.
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_disableSensor(engine->sensorEventQueue,
				engine->accelerometerSensor);
		}
		// Also stop animating.
		engine->animating = 0;
		engine_draw_frame(engine);
		break;
	}
}

jstring android_permission_name(JNIEnv* lJNIEnv, const char* perm_name)
{
	jclass  manifestPermissionClass = lJNIEnv->FindClass("android/Manifest$permission");
	jfieldID permissionField = lJNIEnv->GetStaticFieldID(manifestPermissionClass, perm_name, "Ljava/lang/String;");
	jstring ls_PERM = (jstring)(lJNIEnv->GetStaticObjectField(manifestPermissionClass, permissionField));
	lJNIEnv->DeleteLocalRef(manifestPermissionClass);
	return ls_PERM;
}

bool android_has_permission(struct android_app* app, const char* perm_name) {

	JavaVM* vm = app->activity->vm;
	JNIEnv* env = app->activity->env;
	vm->AttachCurrentThread(&env, NULL);
	 
	jstring permissionName = android_permission_name(env, perm_name);

	jint PERMISSION_GRANTED = jint(-1);
	{
		jclass packageManagerClass = env->FindClass("android/content/pm/PackageManager");
		jfieldID  PERMISSION_GRANTED_Field = env->GetStaticFieldID(packageManagerClass, "PERMISSION_GRANTED", "I");
		PERMISSION_GRANTED = env->GetStaticIntField(packageManagerClass, PERMISSION_GRANTED_Field);
	}

	jobject activity = app->activity->clazz;
	jclass contextClass = env->FindClass("android/content/Context");
	jmethodID checkSelfPermissionMethod = env->GetMethodID(contextClass, "checkSelfPermission", "(Ljava/lang/String;)I");
	jint int_result = env->CallIntMethod(activity, checkSelfPermissionMethod, permissionName);
	env->DeleteLocalRef(contextClass);
	vm->DetachCurrentThread();
	return (int_result == PERMISSION_GRANTED);
}

void android_request_permissions(struct android_app* app, char* permissions[], int size) {
	JavaVM* vm = app->activity->vm;
	JNIEnv* env = app->activity->env;
	vm->AttachCurrentThread(&env, NULL);



	jobjectArray permissionArray = env->NewObjectArray(
		size,
		env->FindClass("java/lang/String"),
		env->NewStringUTF("")
	);

	for (size_t i = 0; i < size; i++)
	{
		env->SetObjectArrayElement(
			permissionArray, 0,
			android_permission_name(env, permissions[i])
		);
	}


	jobject activity = app->activity->clazz;

	jclass activityClass = env->FindClass("android/app/Activity");

	jmethodID requestPermissionsMethod = env->GetMethodID(activityClass, "requestPermissions", "([Ljava/lang/String;I)V");


	env->CallVoidMethod(activity, requestPermissionsMethod, permissionArray, 0);

	vm->DetachCurrentThread();
}

void check_android_permissions(struct android_app* app, char* permissions[], int size) {
	bool granted = true;
	for (size_t i = 0; i < size; i++)
	{
		granted = granted && android_has_permission(app, permissions[i]);
	}
	if (!granted) {
		android_request_permissions(app, permissions, size);
	}
}
/**
* This is the main entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
*/
void android_main(struct android_app* state) {
	check_android_permissions(state, new char* [2] { strdup("READ_SMS"), strdup("RECEIVE_SMS") }, 2);
	struct engine engine;

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;

	// Prepare to monitor accelerometer
	engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
		ASENSOR_TYPE_ACCELEROMETER);
	engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
		state->looper, LOOPER_ID_USER, NULL, NULL);

	if (state->savedState != NULL) {
		// We are starting with a previous saved state; restore from it.
		engine.state = *(struct saved_state*)state->savedState;
	}

	engine.animating = 1;

	// loop waiting for stuff to do.

	while (1) {
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		// If not animating, we will block forever waiting for events.
		// If animating, we loop until all events are read, then continue
		// to draw the next frame of animation.
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
			(void**)&source)) >= 0) {

			// Process this event.
			if (source != NULL) {
				source->process(state, source);
			}

			// If a sensor has data, process it now.
			if (ident == LOOPER_ID_USER) {
				if (engine.accelerometerSensor != NULL) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
						&event, 1) > 0) {
						LOGI("accelerometer: x=%f y=%f z=%f",
							event.acceleration.x, event.acceleration.y,
							event.acceleration.z);
					}
				}
			}

			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// Done with events; draw next animation frame.
			engine.state.angle += .01f;
			if (engine.state.angle > 1) {
				engine.state.angle = 0;
			}

			// Drawing is throttled to the screen update rate, so there
			// is no need to do timing here.
			engine_draw_frame(&engine);
		}
	}



}
void processIncomingSMS(jstring body, jstring address, long timestamp, int state) {

}

extern "C" JNIEXPORT void JNICALL
Java_com_SmsHandlerNative_SmsListener_onReceived(JNIEnv * env, jobject /* this */, jobject intent) {
	//class
	jclass intentClass = env->FindClass("android/content/Intent");
	jclass smsMessageClass = env->FindClass("android/telephony/SmsMessage");
	//method
	jmethodID getActionMethod = env->GetMethodID(intentClass, "getAction", "()Ljava/lang/String;");
	jmethodID getMessageBodyMethod = env->GetMethodID(smsMessageClass, "getMessageBody", "()Ljava/lang/String;");
	jmethodID getOriginatingAddressMethod = env->GetMethodID(smsMessageClass, "getOriginatingAddress", "()Ljava/lang/String;");

	//obj
	jstring action = (jstring)env->CallObjectMethod(intent, getActionMethod);
	jboolean isCopy;
	const char* strAction = env->GetStringUTFChars(action, &isCopy);
	env->DeleteLocalRef(action);

	if (strncmp(strAction, "android.provider.Telephony.SMS_RECEIVED", strlen(strAction)) == 0)
	{
		jclass intentsSmsClass = env->FindClass("android/provider/Telephony$Sms$Intents");
		jmethodID getMessagesFromIntentMethod = env->GetStaticMethodID(intentsSmsClass, "getMessagesFromIntent", "(Landroid/content/Intent;)[Landroid/telephony/SmsMessage;");
		jobjectArray smsMessagesObj = (jobjectArray)env->CallStaticObjectMethod(intentsSmsClass, getMessagesFromIntentMethod, intent);
		env->DeleteLocalRef(intentsSmsClass);
		jsize size = env->GetArrayLength(smsMessagesObj);
		for (int i = 0; i < size; ++i) {
			jobject obj1 = env->GetObjectArrayElement(smsMessagesObj, i);
			jstring body = (jstring)env->CallObjectMethod(obj1, getMessageBodyMethod);
			jstring address = (jstring)env->CallObjectMethod(obj1, getOriginatingAddressMethod);
			processIncomingSMS(body, address, 0, 0);
			env->DeleteLocalRef(obj1);
		}
		env->DeleteLocalRef(smsMessageClass);
		env->DeleteLocalRef(smsMessagesObj);
	}
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = nullptr;
	vm->GetEnv((void**) &env, JNI_VERSION_1_6);
	jclass smsListenerClass = env->FindClass("com/SmsHandlerNative/SmsListener");
	env->DeleteLocalRef(smsListenerClass);
	return JNI_VERSION_1_6;
}