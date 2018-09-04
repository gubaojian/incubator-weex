package com.taobao.weex.render.lifecycle;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import com.taobao.weex.render.manager.RenderManager;

/**
 * Created by furture on 2018/8/31.
 */

public class RenderActivityLifecycleCallback implements Application.ActivityLifecycleCallbacks{


    @Override
    public void onActivityCreated(Activity activity, Bundle savedInstanceState) {

    }

    @Override
    public void onActivityStarted(Activity activity) {

    }

    @Override
    public void onActivityResumed(Activity activity) {
        RenderManager.getInstance().onActivityResumed(activity);
    }

    @Override
    public void onActivityPaused(Activity activity) {
        RenderManager.getInstance().onActivityPaused(activity);
    }

    @Override
    public void onActivityStopped(Activity activity) {

    }

    @Override
    public void onActivitySaveInstanceState(Activity activity, Bundle outState) {

    }

    @Override
    public void onActivityDestroyed(Activity activity) {
        RenderManager.getInstance().onActivityDestroyed(activity);
    }
}
