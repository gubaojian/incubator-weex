/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
package com.taobao.weex.render.manager;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;

import com.taobao.weex.render.image.FrameImageCache;
import com.taobao.weex.render.threads.FrameThread;
import com.taobao.weex.render.threads.IoThread;
import com.taobao.weex.render.frame.RenderFrame;

import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Created by furture on 2018/7/24.
 */

public class RenderManager {

    private Map<Long,WeakReference<RenderFrame>> renderFrameMap;
    private Handler uiHandler;
    private Handler frameHandler;
    private AtomicLong atomicLong;
    private Handler ioHandler;


    private RenderManager(){
        renderFrameMap = new ConcurrentHashMap<>();
        uiHandler = new Handler(Looper.getMainLooper());
        frameHandler = new Handler(FrameThread.getThread().getLooper());
        atomicLong = new AtomicLong(0);
    }

    public Handler getUiHandler(){
        return uiHandler;
    }

    public void registerDocument(long key, RenderFrame render){
        WeakReference<RenderFrame> surfaceRenderWeakReference = new WeakReference<RenderFrame>(render);
       renderFrameMap.put(key, surfaceRenderWeakReference);
    }

    public long nextFrameKey(){
        return atomicLong.incrementAndGet();
    }

    public RenderFrame getFrame(long key){
        WeakReference<RenderFrame> surfaceRenderWeakReference = renderFrameMap.get(key);
        if(surfaceRenderWeakReference == null){
            return null;
        }
        return surfaceRenderWeakReference.get();
    }

    public Map<Long,WeakReference<RenderFrame>> getRenderFrameMap(){
         return renderFrameMap;
    }


    public void onActivityResumed(Activity activity) {
        Set<Map.Entry<Long,WeakReference<RenderFrame>>> entries = renderFrameMap.entrySet();
        for(Map.Entry<Long,WeakReference<RenderFrame>> entry : entries){
            RenderFrame renderFrame = entry.getValue().get();
            if(renderFrame == null){
                continue;
            }
            if(isAcivityDocument(renderFrame, activity)){
                renderFrame.invalidate();
            }
        }
    }

    public void onActivityPaused(Activity activity) {

    }

    public void onActivityDestroyed(Activity activity) {
        Set<Map.Entry<Long,WeakReference<RenderFrame>>> entries = renderFrameMap.entrySet();
        for(Map.Entry<Long,WeakReference<RenderFrame>> entry : entries){
            RenderFrame renderFrame = entry.getValue().get();
            if(renderFrame == null){
                continue;
            }
            if(isAcivityDocument(renderFrame, activity)){
                renderFrame.destory();
            }
        }
    }


    public void onLowMemory() {
        Set<Map.Entry<Long,WeakReference<RenderFrame>>> entries = renderFrameMap.entrySet();
        for(Map.Entry<Long,WeakReference<RenderFrame>> entry : entries){
            RenderFrame renderFrame = entry.getValue().get();
            if(renderFrame == null){
                continue;
            }
            renderFrame.getFrameImageMap().clear();
        }
        FrameImageCache.getInstance().clear();
    }

    private boolean isAcivityDocument(RenderFrame documentView, Activity activity){
        if(documentView.getContext() == activity || documentView.getContext() == activity.getBaseContext()){
            return true;
        }
        return false;
    }



    public void removeDocument(long key){
        renderFrameMap.remove(key);
    }

    public Handler getFrameHandler() {
        return frameHandler;
    }

    public Handler getIoHandler(){
        if(ioHandler == null){
            synchronized (this){
                if(ioHandler == null) {
                    ioHandler = new Handler(IoThread.getThread().getLooper());
                }
            }
        }
        return ioHandler;
    }

    public static RenderManager getInstance(){
        if(renderManager == null){
            synchronized (RenderManager.class){
                if(renderManager == null) {
                    renderManager = new RenderManager();
                }
            }
        }
        return renderManager;
    }
    private static RenderManager renderManager;

}
