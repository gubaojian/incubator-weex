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
package com.taobao.weex.render.bridge;

import android.graphics.Bitmap;
import android.os.Build;
import android.text.TextUtils;
import android.view.Surface;

import com.taobao.weex.render.sdk.RenderSDK;
import com.taobao.weex.render.image.FrameImage;
import com.taobao.weex.render.image.FrameImageCache;
import com.taobao.weex.render.manager.RenderManager;
import com.taobao.weex.render.threads.FrameThread;
import com.taobao.weex.render.frame.RenderFrame;

import java.io.UnsupportedEncodingException;
import java.util.Collection;
import java.util.Map;
import java.util.Set;

/**
 * Created by furture on 2018/7/24.
 */

public class RenderBridge {

    private static final RenderBridge renderBridge = new RenderBridge();

    public static RenderBridge getInstance(){
        return renderBridge;
    }

    /**
     * @param documentKey documentView key
     * @param url imageUrl
     * @param width content image layout width
     * @param height content image layout height
     * the method is called in gpu thread
     * */
    public static Bitmap getImage(long documentKey, String ref, String url, int width, int height, boolean isPlaceholder){
        return RenderBridge.getInstance().getBitmap(documentKey, ref, url, width, height, isPlaceholder);
    }


    public static boolean isImagePremultiplied(Bitmap bitmap){
        if(bitmap == null){
            return true;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            return bitmap.isPremultiplied();
        }
        return  true;
    }

    public boolean initSDK(int screenWidth, int screenHeight, float density){
        return nativeInitSDK(screenWidth, screenHeight, density);
    }


    public long createFrame(long renderKey) {
        return nativeCreateFrame(renderKey);
    }

    public void destroyFrame(long mNativeFrame) {
        nativeDestroyFrame(mNativeFrame);
    }


    public boolean paintFrame(long mNativeDocument, long mFrameRender) {
        return nativePaintFrame(mNativeDocument, mFrameRender);
    }


    public void layoutFrame(long ptr){
        nativeLayoutFrame(ptr);
    }


    public void createBody(long ptr, String ref, Map<String,String> styles, Map<String,String> attrs, Collection<String> events){
        nativeCreateBody(ptr, ref, toNativeMap(styles), toNativeMap(attrs), toNativeSet(events));
    }

    public void actionAddElement(long ptr, String ref, String type, String parentRef, int index, Map<String,String> styles, Map<String,String> attrs, Collection<String> events){
        nativeActionAddElement(ptr, ref, type, parentRef, index, toNativeMap(styles), toNativeMap(attrs), toNativeSet(events));
    }

    public void actionUpdateAttrs(long nativeDocument, String ref, Map<String, String> attrs) {
        nativeActionUpdateAttrs(nativeDocument, ref, toNativeMap(attrs));
    }

    public void actionUpdateStyles(long nativeDocument, String ref, Map<String, String> styles) {
        nativeActionUpdateStyles(nativeDocument, ref, toNativeMap(styles));
    }

    public void actionAddEvent(long nativeDocument, String ref, String event) {
        if(TextUtils.isEmpty(event) || TextUtils.isEmpty(ref)){
            return;
        }
        nativeActionAddEvent(nativeDocument, ref, event);
    }

    public void actionRemoveEvent(long nativeDocument, String ref, String event) {
        if(TextUtils.isEmpty(event) || TextUtils.isEmpty(ref)){
            return;
        }
        nativeActionRemoveEvent(nativeDocument, ref, event);
    }


    public void actionRemoveElement(long nativeDocument, String ref) {
        nativeActionRemoveElement(nativeDocument, ref);
    }


    public void actionMoveElement(long nativeDocument, String ref, String parentRef, int index){
        nativeActionMoveElement(nativeDocument, ref, parentRef, index);
    }

    public void actionRefreshFont(final long nativeDocument, final String fontFamilyName){
        if(Thread.currentThread() != FrameThread.getThread()){
            RenderManager.getInstance().getFrameHandler().post(new Runnable() {
                @Override
                public void run() {
                    actionRefreshFont(nativeDocument, fontFamilyName);
                }
            });
            return;
        }
        nativeRefreshFont(nativeDocument, fontFamilyName);
    }


    public String actionHitTestEvent(long mNativeFrame, int type, int x, int y) {
        return nativeHitTestEvent(mNativeFrame, type, x, y);
    }


    public void addFont(final String fontFamilyName, final String fontPath){
        if(Thread.currentThread() != FrameThread.getThread()){
            RenderManager.getInstance().getFrameHandler().post(new Runnable() {
                @Override
                public void run() {
                    addFont(fontFamilyName, fontPath);
                }
            });
            return;
        }
        nativeAddFont(fontFamilyName, fontPath);
    }


    public boolean hasFont(final String fontFamilyName, final String fontPath){
        return nativeHasFont(fontFamilyName, fontPath);
    }


    public long frameRenderAttach(Surface surface, int width, int height){
        return nativeFrameRenderAttach(surface, width, height);
    }

    public void frameRenderInvalidate(long nativeFrameRender){
        nativeFrameRenderInvalidate(nativeFrameRender);
    }

    public void frameRenderDettach(long nativeFrameRender, Surface surface){
        nativeFrameRenderDettach(nativeFrameRender, surface);
    }

    public void frameRenderClearBuffer(long nativeFrameRender) {
        nativeFrameRenderClearBuffer(nativeFrameRender);
    }

    public boolean frameRenderSwap(long nativeFrameRender) {
        if(nativeFrameRender == 0){
            throw new RuntimeException("nativeFrameRender has been released");
        }
        return nativeFrameRenderSwap(nativeFrameRender);
    }


    public void frameRenderSizeChanged(long nativeFrameRender, int width, int height) {
        nativeFrameRenderSizeChanged(nativeFrameRender, width, height);
    }


    public int frameWidth(long ptr){
        return nativeRenderFrameWidth(ptr);
    }
    public int frameHeight(long ptr){
        return nativeRenderFrameHeight(ptr);
    }

    public int getBlockLayout(long ptr, String ref, int edge){
        return nativeGetBlockLayout(ptr, ref, edge);
    }

    public String getNodeAttr(long ptr, String ref, String key){
        byte [] bts =  nativeGetNodeAttr(ptr, ref, key);
        if(bts == null){
            return null;
        }
        try {
            return new String(bts, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            return null;
        }
    }

    public String getNodeType(long ptr, String ref){
        return nativeGetNodeType(ptr, ref);
    }

    public boolean nodeContainsEvent(long ptr, String ref, String event){
        return nativeNodeContainsEvent(ptr, ref, event);
    }

    public long toNativeMap(Map<String,String> styles){
        long map = nativeNewMap();
        if(styles != null){
            Set<Map.Entry<String,String>> entrySet = styles.entrySet();
            for(Map.Entry<String,String> entry : entrySet){
                if(entry.getValue() == null){
                    continue;
                }
                nativeMapPut(map, entry.getKey(), entry.getValue());
            }
        }
        return map;
    }

    public long toNativeSet(Collection<String> events){
        long set = nativeNewSet();
        if(events != null){
            for(String envent : events){
                nativeSetAdd(set, envent);
            }
        }
        return set;
    }




    public Bitmap getBitmap(long documentKey, String ref, final String url, int width, int height, boolean isPlaceholder){
        if(TextUtils.isEmpty(url)){
            return null;
        }
        if(RenderSDK.getInstance().getImageAdapter() == null){
            return null;
        }
        RenderFrame renderFrame = RenderManager.getInstance().getFrame(documentKey);
        if(renderFrame == null
                || renderFrame.getFrameRender() == null){
            return null;
        }
        String imageKey = RenderSDK.getInstance().getImageAdapter().genImageKey(renderFrame, ref, url, width, height, isPlaceholder);
        FrameImage target = renderFrame.getFrameImageMap().get(imageKey);
        if(target == null){
            target = FrameImageCache.getInstance().getFrameImage(documentKey, imageKey);
            if(target != null){
                renderFrame.getFrameImageMap().put(imageKey, target);
            }
        }
        if(target != null) {
            if (target.getLoadingState() == FrameImage.LOADING) {
                return null;
            }
            if (target.getLoadingState() == FrameImage.LOADING_SUCCESS) {
                return target.getBitmap();
            }
        }
        FrameImage loadImageTarget = RenderSDK.getInstance().getImageAdapter().requestFrameImage(renderFrame, ref, url, width, height, isPlaceholder);
        if(loadImageTarget == null || imageKey == null){
            return null;
        }
        renderFrame.getFrameImageMap().put(imageKey, loadImageTarget);
        return loadImageTarget.getBitmap();
    }



    private native boolean nativeInitSDK(int screenWidth, int screenHeight, float density);
    private native long nativeCreateFrame(long key);
    private native boolean nativePaintFrame(long ptr, long mFrameRender);
    private native void nativeLayoutFrame(long ptr);
    private native String nativeHitTestEvent(long mNativeDocument, int type, int x, int  y);
    private native void nativeFrameRenderClearBuffer(long nativeFrameRenderPtr);
    private native void nativeFrameRenderInvalidate(long nativeFrameRenderPtr);
    private native boolean nativeFrameRenderSwap(long nativeFrameRenderPtr);
    private native void nativeDestroyFrame(long ptr);
    private native int nativeRenderFrameWidth(long ptr);
    private native int nativeRenderFrameHeight(long ptr);
    private native long nativeFrameRenderAttach(Surface surface, int width, int height);
    private native void nativeFrameRenderSizeChanged(long nativeFrameRender, int width, int height);
    private native void nativeFrameRenderDettach(long nativeFrameRender, Surface surface);
    private native void nativeCreateBody(long ptr, String ref, long styles, long attrs, long events);
    private native void nativeActionAddElement(long ptr, String ref, String type, String parentRef, int index, long styles, long attrs, long events);
    private native void nativeActionUpdateAttrs(long ptr, String ref, long attrs);
    private native void nativeActionUpdateStyles(long ptr, String ref, long styles);
    private native void nativeActionAddEvent(long nativeDocument, String ref, String event);
    private native void nativeActionRemoveEvent(long nativeDocument, String ref, String event);
    private native void nativeActionRemoveElement(long nativeDocument, String ref);
    private native void nativeActionMoveElement(long nativeDocument, String ref, String parentRef, int index);
    private native void nativeRefreshFont(long nativeDocument, String fontFamilyName);
    private native void nativeAddFont(String fontFamilyName, String fontPath);
    private native boolean nativeHasFont(String fontFamilyName, String fontPath);
    private native int nativeGetBlockLayout(long ptr, String ref, int edge);
    private native byte[] nativeGetNodeAttr(long ptr, String ref, String key);
    private native String nativeGetNodeType(long ptr, String ref);
    private native boolean nativeNodeContainsEvent(long ptr, String ref, String event);
    private native long nativeNewMap();
    private native void nativeMapPut(long ptr, String key, String value);
    private native long nativeNewSet();
    private native void nativeSetAdd(long ptr, String value);
}
