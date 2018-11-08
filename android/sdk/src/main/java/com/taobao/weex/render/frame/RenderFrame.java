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
package com.taobao.weex.render.frame;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.taobao.weex.render.action.DestoryFrameAction;
import com.taobao.weex.render.action.HitTestEventAction;
import com.taobao.weex.render.action.RefreshFontAction;
import com.taobao.weex.render.event.OnFrameSizeChangedListener;
import com.taobao.weex.render.sdk.RenderSDK;
import com.taobao.weex.render.action.AddElementAction;
import com.taobao.weex.render.action.AddEventAction;
import com.taobao.weex.render.action.CreateBodyAction;
import com.taobao.weex.render.action.MoveElementAction;
import com.taobao.weex.render.action.RemoveElementAction;
import com.taobao.weex.render.action.RemoveEventAction;
import com.taobao.weex.render.action.UpdateAttrsAction;
import com.taobao.weex.render.action.UpdateStylesAction;
import com.taobao.weex.render.bridge.RenderBridge;
import com.taobao.weex.render.event.RenderFrameAdapter;
import com.taobao.weex.render.image.FrameImage;
import com.taobao.weex.render.image.FrameImageCache;
import com.taobao.weex.render.log.RenderLog;
import com.taobao.weex.render.threads.FrameThread;
import com.taobao.weex.render.manager.RenderManager;

import java.util.Collection;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;


/**
 * Sureface Render For OpenGL Context
 * Created by furture on 2018/7/23.
 */
public class RenderFrame implements  FrameController.FrameListener {

    private long mNativeFrame;
    private final long mFrameKey;
    private boolean mDestroy;
    private volatile boolean mPause;
    private int frameHeight;
    private int frameWidth;
    private Context mContext;
    private Handler frameHandler;
    private Handler mainHandler;
    private RenderFrameRender mFrameRender;
    private Map<String, FrameImage> frameImageMap;
    private RenderFrameAdapter mFrameAdapter;
    private Object instance;
    private FrameController mFrameController;



    public RenderFrame(Context context){
        this.mContext = context;
        mFrameKey = RenderManager.getInstance().nextFrameKey();
        RenderManager.getInstance().registerDocument(mFrameKey, this);
        mainHandler = new Handler(Looper.getMainLooper());
        frameHandler = new Handler(FrameThread.getThread().getLooper());
        if(!RenderSDK.getInstance().isInited()){
            RenderSDK.getInstance().initSync();
        }
        frameImageMap = new ConcurrentHashMap<>();
        mPause = true;
        mFrameController = new FrameController(frameHandler, this);
    }


    public void actionCreateBody(String ref, Map<String, String> style, Map<String, String> attrs, Collection<String> events){
        CreateBodyAction createBodyAction = new CreateBodyAction(this, ref, style, attrs, events);
        if(frameHandler != null){
            frameHandler.post(createBodyAction);
        }
    }

    public void actionAddElement(String ref, String componentType, String parentRef, int index, Map<String, String> style, Map<String, String> attrs, Collection<String> events){
        AddElementAction addElementAction = new AddElementAction(this, ref, componentType, parentRef, index, style, attrs, events);
        if(frameHandler != null){
            frameHandler.post(addElementAction);
        }
    }

    public void actionUpdateAttrs(String ref, Map<String, String> attrs){
        UpdateAttrsAction updateAttrsAction = new UpdateAttrsAction(this, ref, attrs);
        if(frameHandler != null){
            frameHandler.post(updateAttrsAction);
        }
    }

    public void actionUpdateStyles(String ref, Map<String, String> styles){
        if(styles == null || styles.size() <= 0){
            return;
        }
        UpdateStylesAction updateStylesAction = new UpdateStylesAction(this, ref, styles);
        if(frameHandler != null){
            frameHandler.post(updateStylesAction);
        }
    }

    public void actionAddEvent(String ref, String event){
        AddEventAction addEventAction = new AddEventAction(this, ref, event);
        if(frameHandler != null){
            frameHandler.post(addEventAction);
        }
    }

    public void actionRemoveEvent(String ref, String event){
        RemoveEventAction removeEventAction = new RemoveEventAction(this, ref, event);
        if(frameHandler != null){
            frameHandler.post(removeEventAction);
        }
    }


    public void actionMoveElement(String ref, String parentRef, int index){
        MoveElementAction moveElementAction = new MoveElementAction(this, ref, parentRef, index);
        if(frameHandler != null){
            frameHandler.post(moveElementAction);
        }
    }


    public void actionRemoveElement(String ref){
        RemoveElementAction removeElementAction = new RemoveElementAction(this, ref);
        if(frameHandler != null){
            frameHandler.post(removeElementAction);
        }
    }

    public RenderFrameRender getFrameRender(){
        return mFrameRender;
    }


    public void requestLayout(){
        mFrameController.requestFrame();
    }


    public void requestFrame(){
        mFrameController.requestFrame();
    }

    public void invalidate(){
        if(mPause || mFrameRender == null){
            return;
        }
        mFrameRender.invalidate();
    }


    public void actionHitTestEvent(int type, float x, float y){
        HitTestEventAction hitTestEventAction = new HitTestEventAction(this, type, (int)x, (int)y);
        frameHandler.post(hitTestEventAction);
    }

    public void actionRefreshFont(String fontFaimly) {
        RefreshFontAction refreshFontAction = new RefreshFontAction(this, fontFaimly);
        frameHandler.post(refreshFontAction);
    }

    /**
     * when load image target complete, call this method
     * */
    public void onLoadFrameImage(FrameImage frameImage){
        if(frameImage == null){
            return;
        }
        if(frameImage.getBitmap() != null){
            requestFrame();
        }
        if(frameImage.isPlaceholder()){
            return;
        }
        if(mFrameAdapter != null && mFrameAdapter.getImgLoadListener() != null){
            mFrameAdapter.getImgLoadListener().onLoadImage(frameImage);
        }
    }


    @Override
    public void onFrame() {
        if(mDestroy){
            return;
        }
        if(mNativeFrame == 0){
            return;
        }

        layoutFrame();

        if(mDestroy){
            return;
        }

        boolean paint = paintFrame();

        if(mDestroy){
            return;
        }
        if(paint){
            invalidate();
        }
    }


    private void layoutFrame() {
        if(mNativeFrame != 0){
            RenderLog.actionLayoutExecute(this);
            RenderBridge.getInstance().layoutFrame(mNativeFrame);
            int height = RenderBridge.getInstance().frameHeight(mNativeFrame);
            int width = RenderBridge.getInstance().frameWidth(mNativeFrame);
            setRenderFrameSize(width, height);
        }
    }


    private boolean paintFrame(){
        if(mDestroy){
            return false;
        }
        if(mPause){
            return false;
        }

        if(mNativeFrame == 0){
            return false;
        }

        RenderFrameRender renderFrameRender = mFrameRender;
        if(renderFrameRender== null){
            return false;
        }

        synchronized (renderFrameRender){
            long mNativeFrameRender  = renderFrameRender.getNativeFrameRender();
            if(mNativeFrameRender == 0){
                return false;
            }
            return RenderBridge.getInstance().paintFrame(mNativeFrame, mNativeFrameRender);
        }
    }



    public Context getContext() {
        return  mContext;
    }

    public long getFrameKey() {
        return mFrameKey;
    }

    public long getNativeFrame() {
        return mNativeFrame;
    }

    public void setNativeFrame(long nativeFrame) {
        this.mNativeFrame = nativeFrame;
    }

    public Map<String, FrameImage> getFrameImageMap() {
        return frameImageMap;
    }

    public boolean isPause() {
        return mPause;
    }


    public void setPause(boolean mPause) {
        if(this.mPause != mPause){
            this.mPause = mPause;
            this.mFrameRender = null;
            if(mPause){
                FrameImageCache.getInstance().cacheFrameImages(mFrameKey, frameImageMap);
                frameImageMap.clear();
            }
        }
    }


    public int getFrameHeight() {
        return frameHeight;
    }

    public int getFrameWidth() {
        return frameWidth;
    }

    public Object getInstance() {
        return instance;
    }

    public void setInstance(Object instance) {
        this.instance = instance;
    }

    public void setFrameAdapter(RenderFrameAdapter documentAdapter) {
        this.mFrameAdapter = documentAdapter;
    }

    public RenderFrameAdapter getFrameAdapter() {
        return mFrameAdapter;
    }


    private void setRenderFrameSize(final int width, final int height) {
        if(width <= 0 || height <= 0){
            return;
        }
        if(frameWidth == width
                && frameHeight == height){
            return;
        }

        frameWidth = width;
        frameHeight = height;
        final RenderFrameAdapter frameAdapter = mFrameAdapter;
        if(frameAdapter == null){
            return;
        }
        final OnFrameSizeChangedListener frameSizeChangedListener = frameAdapter.getDocumentSizeChangedListener();
        if(frameSizeChangedListener == null){
            return;
        }
        if(!frameAdapter.isRequireFrameSizeChangedOnMainThread()){
            frameSizeChangedListener.onSizeChanged(RenderFrame.this, width, height);
            return;
        }
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                frameSizeChangedListener.onSizeChanged(RenderFrame.this, width, height);
            }
        });
    }



    public  void destory(){
        synchronized (this){
            if(!isDestroy()){
                frameHandler.post(new DestoryFrameAction(this));
                RenderManager.getInstance().removeDocument(mFrameKey);
                mFrameAdapter = null;
                mFrameRender = null;
                mContext = null;
                frameImageMap.clear();
                mPause = true;
                mDestroy = true;
            }
        }
    }

    public boolean isDestroy(){
        return mDestroy;
    }

    @Override
    protected void finalize() throws Throwable {
        if(mNativeFrame != 0){
            if(!isDestroy()){
                destory();
            }
        }
        super.finalize();
    }

    public void setFrameRender(RenderFrameRender mFrameRender) {
        this.mFrameRender = mFrameRender;
    }
}
