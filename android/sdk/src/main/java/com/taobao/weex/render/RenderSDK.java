package com.taobao.weex.render;

import android.app.Application;
import android.util.DisplayMetrics;

import com.taobao.weex.render.bridge.RenderBridge;
import com.taobao.weex.render.image.RenderImageAdapter;
import com.taobao.weex.render.lifecycle.RenderActivityLifecycleCallback;
import com.taobao.weex.render.manager.RenderManager;
import com.taobao.weex.render.threads.GpuThread;
import com.taobao.weex.render.threads.IoThread;

/**
 * Created by furture on 2018/8/31.
 */

public class RenderSDK {

    private static final String WEEX_RENDER = "weexrender";

    private Application application;
    private RenderImageAdapter imageAdapter;
    private boolean isInited = false;
    private RenderActivityLifecycleCallback renderActivityLifecycleCallback;


    private static RenderSDK renderSDK;

    private RenderSDK(){
        renderActivityLifecycleCallback = new RenderActivityLifecycleCallback();
    }

    public static RenderSDK getInstance(){
        if(renderSDK == null){
            synchronized (RenderSDK.class){
                if(renderSDK == null){
                    renderSDK = new RenderSDK();
                }
            }
        }
        return renderSDK;
    }


    public Application getApplication() {
        return application;
    }

    public RenderSDK setApplication(Application application) {
        this.application = application;
        return this;
    }

    public RenderImageAdapter getImageAdapter() {
        return imageAdapter;
    }

    public RenderSDK setImageAdapter(RenderImageAdapter imageAdapter) {
        this.imageAdapter = imageAdapter;
        return this;
    }

    public void init(){
        RenderManager.getInstance().getGpuHandler().post(new Runnable() {
            @Override
            public void run() {
                initSdk();
            }
        });
    }


    public boolean isInited(){
        return isInited;
    }


    private void initSdk(){
        if(!isInited){
            synchronized (this){
                if(isInited){
                    return;
                }
                if(Thread.currentThread() != GpuThread.getThread()){
                    throw new RuntimeException("RenderSDK InitSdk Must be called on GPU Thread");
                }
                if(application == null){
                    throw new RuntimeException("RenderSDK setApplication Should be called When Application Init");
                }
                application.unregisterActivityLifecycleCallbacks(renderActivityLifecycleCallback);
                application.registerActivityLifecycleCallbacks(renderActivityLifecycleCallback);
                System.loadLibrary(WEEX_RENDER);
                DisplayMetrics displayMetrics = application.getResources().getDisplayMetrics();
                RenderBridge.getInstance().initSDK(displayMetrics.widthPixels, displayMetrics.heightPixels, displayMetrics.density);
                isInited = true;
            }
        }
    }

    public void destory(){
        try{
            application.unregisterActivityLifecycleCallbacks(renderActivityLifecycleCallback);
            GpuThread.getThread().quit();
            IoThread.getThread().quit();
        }catch (Exception e){}
    }
}
