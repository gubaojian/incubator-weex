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
package com.taobao.weex.render.view;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;

import com.taobao.weex.render.RenderSDK;
import com.taobao.weex.render.action.AddElementAction;
import com.taobao.weex.render.action.AddEventAction;
import com.taobao.weex.render.action.CreateBodyAction;
import com.taobao.weex.render.action.MoveElementAction;
import com.taobao.weex.render.action.RemoveElementAction;
import com.taobao.weex.render.action.RemoveEventAction;
import com.taobao.weex.render.action.UpdateAttrsAction;
import com.taobao.weex.render.action.UpdateStylesAction;
import com.taobao.weex.render.bridge.RenderBridge;
import com.taobao.weex.render.event.DocumentAdapter;
import com.taobao.weex.render.event.OnEventListener;
import com.taobao.weex.render.image.BitmapTarget;
import com.taobao.weex.render.image.RenderBitmapImageCache;
import com.taobao.weex.render.threads.GpuThread;
import com.taobao.weex.render.manager.RenderManager;
import com.taobao.weex.render.threads.WeakRefHandler;
import com.taobao.weex.render.task.OpenGLRender;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;


/**
 * Sureface Render For OpenGL Context
 * Created by furture on 2018/7/23.
 */
public class DocumentView implements Handler.Callback {
    /**
     * Weex Render MSG TYPE on Gpu Thread
     * */
    public static final int MSG_CREATE_DOCUMENT = 0X02;
    public static final int MSG_ATTACH_GL_RENDER = 0X03;
    public static final int MSG_RENDER_SIZE_CHANGED = 0X04;
    public static final int MSG_RENDER_INVALIDATE = 0X05;
    public static final int MSG_DETACH_GL_RENDER = 0X06;
    public static final int MSG_DESTORY_DOCUMENT = 0X07;
    public static final int MSG_RENDER_ACTION_EVENT = 0X08;
    public static final int MSG_RENDER_LAYOUT = 0X09;
    public static final int MSG_RENDER_REFRESH_FONT = 0X10;



    private long mNativeDocument;
    private final long mDocumentKey;
    private boolean destroy;
    private volatile boolean mPause;
    private int documentHeight;
    private int documentWidth;
    private Context mContext;
    private Handler gpuHandler;
    private Handler mainHandler;
    private OpenGLRender mOpenGLRender;
    private Map<String, BitmapTarget> imageTargetMap;
    private FrameTask frameTask;
    private AtomicInteger renderStage;
    private FrameTask layoutTask;
    private DocumentAdapter mDocumentAdapter;
    private Object instance;
    public Object lock;


    public DocumentView(Context context){
        this.mContext = context;
        this.lock = new Object();
        mDocumentKey = RenderManager.getInstance().nextDocumentKey();
        RenderManager.getInstance().registerDocument(mDocumentKey, this);
        mainHandler = new Handler(Looper.getMainLooper());
        gpuHandler = new WeakRefHandler(GpuThread.getThread().getLooper(), this);
        if(!RenderSDK.getInstance().isInited()){
            RenderSDK.getInstance().init();
        }
        gpuHandler.sendEmptyMessage(MSG_CREATE_DOCUMENT);
        imageTargetMap = new ConcurrentHashMap<>();
        renderStage = new AtomicInteger();
        mPause = true;
    }



    public Context getContext() {
        return  mContext;
    }

    public long getDocumentKey() {
        return mDocumentKey;
    }

    public long getNativeDocument() {
        return mNativeDocument;
    }

    public Map<String, BitmapTarget> getImageTargetMap() {
        return imageTargetMap;
    }

    public AtomicInteger getRenderStage() {
        return renderStage;
    }

    public boolean isPause() {
        return mPause;
    }



    public void setPause(boolean mPause) {
        synchronized (lock){
            if(this.mPause != mPause){
                this.mPause = mPause;
                this.mOpenGLRender = null;
                this.renderStage.incrementAndGet();
                if(mPause){
                    if(frameTask != null){
                        gpuHandler.removeCallbacks(frameTask);
                        frameTask = null;
                    }
                    RenderBitmapImageCache.getInstance().cacheImageTargets(mDocumentKey, imageTargetMap);
                    imageTargetMap.clear();
                }
            }
        }
    }


    public void actionCreateBody(String ref, Map<String, String> style, Map<String, String> attrs, Collection<String> events){
        CreateBodyAction createBodyAction = new CreateBodyAction(this, ref, style, attrs, events);
        if(gpuHandler != null){
            gpuHandler.post(createBodyAction);
        }
    }

    public void actionAddElement(String ref, String componentType, String parentRef, int index, Map<String, String> style, Map<String, String> attrs, Collection<String> events){
        if(style != null){
            style = new HashMap<String, String>(style);
        }
        if(attrs != null){
            attrs = new HashMap<String, String>(attrs);
        }
        if(events != null){
           events =  new ArrayList<String>(events);
        }
        AddElementAction addElementAction = new AddElementAction(this, ref, componentType, parentRef, index, style, attrs, events);
        if(gpuHandler != null){
            gpuHandler.post(addElementAction);
        }
    }

    public void actionUpdateAttrs(String ref, Map<String, String> attrs){
        UpdateAttrsAction updateAttrsAction = new UpdateAttrsAction(this, ref, attrs);
        if(gpuHandler != null){
            gpuHandler.post(updateAttrsAction);
        }
    }

    public void actionUpdateStyles(String ref, Map<String, String> styles){
        if(styles == null || styles.size() <= 0){
            return;
        }
        UpdateStylesAction updateStylesAction = new UpdateStylesAction(this, ref, styles);
        if(gpuHandler != null){
            gpuHandler.post(updateStylesAction);
        }
    }

    public void actionAddEvent(String ref, String event){
        AddEventAction addEventAction = new AddEventAction(this, ref, event);
        if(gpuHandler != null){
            gpuHandler.post(addEventAction);
        }
    }

    public void actionRemoveEvent(String ref, String event){
        RemoveEventAction removeEventAction = new RemoveEventAction(this, ref, event);
        if(gpuHandler != null){
            gpuHandler.post(removeEventAction);
        }
    }


    public void actionMoveElement(String ref, String parentRef, int index){
        MoveElementAction moveElementAction = new MoveElementAction(this, ref, parentRef, index);
        if(gpuHandler != null){
            gpuHandler.post(moveElementAction);
        }
    }


    public void actionRemoveElement(String ref){
        RemoveElementAction removeElementAction = new RemoveElementAction(this, ref);
        if(gpuHandler != null){
            gpuHandler.post(removeElementAction);
        }
    }

    public void attachGLRender(OpenGLRender openGLRender){
        if(this.mOpenGLRender != null){
            this.mOpenGLRender.setWillDestory(true);
            this.mOpenGLRender.destroy();
        }
        this.mOpenGLRender = openGLRender;
        if(openGLRender.getPtr() == 0){
            throw new IllegalArgumentException("DocumentView attachGLRender mOpenGLRender ptr " + openGLRender);
        }
        if(mNativeDocument == 0){
            Message message = Message.obtain(gpuHandler, MSG_ATTACH_GL_RENDER, openGLRender);
            message.sendToTarget();
        }else {
            RenderBridge.getInstance().attachRender(mNativeDocument, openGLRender.getPtr());
            handleMessageInvalidate(renderStage.get());
        }
    }



    public OpenGLRender getOpenGLRender(){
        return mOpenGLRender;
    }

    public boolean hasOpenGLRender(){
        return mOpenGLRender != null;
    }

    public void renderSizeChanged(int width, int height){
        synchronized (lock){
            if( frameTask != null){
                gpuHandler.removeCallbacks(frameTask);
                frameTask = null;
            }
        }
        if(mOpenGLRender != null){
            if(mOpenGLRender.getHeight() != height || mOpenGLRender.getWidth() != width){
                mOpenGLRender.setWidth(width);
                mOpenGLRender.setHeight(height);
                Message message = Message.obtain(gpuHandler, MSG_RENDER_SIZE_CHANGED, width, height, mOpenGLRender.getPtr());
                message.sendToTarget();
            }else{
                invalidate();
            }
        }
    }

    public void invalidate(){
        if(mPause || mOpenGLRender == null){
            return;
        }
        synchronized (lock){
            if(frameTask == null){
                frameTask = new FrameTask() {
                    @Override
                    public void run() {
                        synchronized (lock){
                            Message message = Message.obtain(gpuHandler, MSG_RENDER_INVALIDATE, renderStage.get(), 0);
                            message.sendToTarget();
                            frameTask = null;
                        }
                    }
                };
            }
            gpuHandler.removeCallbacks(frameTask);
            gpuHandler.postDelayed(frameTask, frameTask.frameTime());
        }
    }

    public void requestLayout(){
        Log.e("weex", "Weex Request Layout " + hashCode());
        synchronized (DocumentView.this.lock){
            if(layoutTask == null){
                Log.e("weex", "Weex Request Layout new  task  " + hashCode());
                layoutTask = new FrameTask() {
                    @Override
                    public void run() {
                        synchronized (DocumentView.this.lock){
                            Log.e("weex", "Weex Request Layout send message " + DocumentView.this.hashCode());
                            Message message = Message.obtain(gpuHandler, MSG_RENDER_LAYOUT);
                            message.sendToTarget();
                            layoutTask = null;
                            Log.e("weex", "Weex Request Layout task set null " + DocumentView.this.hashCode());
                        }
                    }
                };
            }
            gpuHandler.removeCallbacks(layoutTask);
            Log.e("weex", "Weex Request Layout frame time " + hashCode());
            gpuHandler.postDelayed(layoutTask, FrameTask.FRAME_TIME);
        }
    }


    public void actionEvent(int type, float x, float y){
        if(mNativeDocument != 0){
            Message message = Message.obtain(gpuHandler, MSG_RENDER_ACTION_EVENT, (int)x, (int)y, type);
            message.sendToTarget();
        }
    }

    public void refreshFont(String fontFaimly) {
        if(TextUtils.isEmpty(fontFaimly)){
            return;
        }
        if(mNativeDocument != 0){
            Message message = Message.obtain(gpuHandler, MSG_RENDER_REFRESH_FONT, fontFaimly);
            message.sendToTarget();
        }
    }

    /**
     * when load image target complete, call this method
     * */
    public void onLoadImageTarget(BitmapTarget target){
        if(target == null){
            return;
        }
        if(target.getBitmap() != null){
            invalidate();
        }
        if(target.isPlaceholder()){
            return;
        }
        if(mDocumentAdapter != null && mDocumentAdapter.getImgLoadListener() != null){
            mDocumentAdapter.getImgLoadListener().onLoadImage(target);
        }
    }

    public  void destory(){
        synchronized (lock){
            if(!isDestroy()){
                Message message = Message.obtain(gpuHandler, MSG_DESTORY_DOCUMENT);
                message.sendToTarget();
                RenderManager.getInstance().removeDocument(mDocumentKey);
                mDocumentAdapter = null;
                mOpenGLRender = null;
                mPause = true;
                destroy = true;
            }
        }
    }

    public boolean isDestroy(){
        return destroy;
    }

    @Override
    protected void finalize() throws Throwable {
        if(mNativeDocument != 0){
            if(!isDestroy()){
                destory();
            }
        }
        super.finalize();
    }


    public void handleMessageInvalidate(int token){
        if(mNativeDocument == 0){
            return;
        }
        if(mPause){
            return;
        }
        synchronized (lock){
            if(frameTask != null){
                gpuHandler.removeCallbacks(frameTask);
                frameTask = null;
            }
        }
        if(mOpenGLRender == null || mOpenGLRender.isWillDestory()){
            return;
        }
        if(token != renderStage.get()){
            return;
        }
        boolean hasInvalidateDraw = RenderBridge.getInstance().invalidate(mNativeDocument);
        if(!hasInvalidateDraw){
            return;
        }
        if(token != renderStage.get()){
            return;
        }
        synchronized (lock){
            if(!mPause){
                if(token == renderStage.get()){
                    RenderBridge.getInstance().swap(mNativeDocument);
                }
            }
        }
    }

    private void setSize(final int width, final int height) {
        if(width <= 0 || height <= 0){
            return;
        }
        if(documentWidth != width || height != documentHeight){
            documentWidth = width;
            documentHeight = height;
            if(mDocumentAdapter != null && mDocumentAdapter.getDocumentSizeChangedListener() != null){
                mainHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if(documentWidth == width && documentHeight == height
                                && mDocumentAdapter != null
                                && mDocumentAdapter.getDocumentSizeChangedListener() != null){
                            mDocumentAdapter.getDocumentSizeChangedListener().onSizeChanged(DocumentView.this, width, height);
                        }
                    }
                });
            }
        }
    }


    @Override
    public boolean handleMessage(Message msg) {
            if(mPause){
                if(msg.what == MSG_RENDER_INVALIDATE || msg.what == MSG_ATTACH_GL_RENDER){
                    return true;
                }
            }
            if(destroy){
                if(msg.what != MSG_DESTORY_DOCUMENT){
                    return true;
                }
            }
            switch (msg.what){
                case MSG_CREATE_DOCUMENT:{
                    if(mNativeDocument != 0){
                        RenderBridge.getInstance().destroyDocument(mNativeDocument);
                        mNativeDocument = 0;
                    }
                    mNativeDocument =  RenderBridge.getInstance().createDocument(mDocumentKey);
                }
                break;
                case MSG_DESTORY_DOCUMENT:{
                    if(mNativeDocument != 0){
                        destroy = true;
                        RenderBridge.getInstance().destroyDocument(mNativeDocument);
                        mDocumentAdapter = null;
                        gpuHandler.removeCallbacks(null);
                        mNativeDocument = 0;
                    }
                }
                break;
                case MSG_ATTACH_GL_RENDER:{
                    if(mNativeDocument != 0 && mOpenGLRender != null){
                        OpenGLRender attachRender = (OpenGLRender) msg.obj;
                        long  renderPtr = attachRender.getPtr();
                        if(renderPtr != 0 && !attachRender.isWillDestory()){
                            RenderBridge.getInstance().attachRender(mNativeDocument, renderPtr);
                            if(!mPause) {
                                handleMessageInvalidate(renderStage.get());
                            }
                        }
                    }
                }
                break;
                case MSG_RENDER_SIZE_CHANGED:{
                    if(mNativeDocument != 0 && mOpenGLRender != null ){
                        long  renderPtr = (long) msg.obj;
                        int width = msg.arg1;
                        int height = msg.arg2;
                        if(mOpenGLRender.getPtr() == renderPtr){
                            RenderBridge.getInstance().renderSizeChanged(mNativeDocument, renderPtr, width, height);
                            if(!mPause){
                                synchronized (lock){
                                    if(!mPause && mOpenGLRender != null && mOpenGLRender.getPtr() == renderPtr) {
                                        RenderBridge.getInstance().swap(mNativeDocument);
                                    }
                                }
                            }
                            if(!mPause && mOpenGLRender != null && mOpenGLRender.getPtr() == renderPtr){
                                handleMessageInvalidate(renderStage.get());
                            }
                        }
                    }
                }
                break;
                case MSG_DETACH_GL_RENDER:{
                    if(mNativeDocument != 0){
                        long  renderPtr = (long) msg.obj;
                        RenderBridge.getInstance().detachRender(mNativeDocument, renderPtr);
                    }
                }
                break;
                case MSG_RENDER_INVALIDATE:{
                    handleMessageInvalidate(msg.arg1);
                }
                break;
                case MSG_RENDER_LAYOUT:{
                    if(mNativeDocument != 0){
                        Log.e("weex", "Weex Request Layout execute " + hashCode());
                        RenderBridge.getInstance().layoutIfNeed(mNativeDocument);
                        int height = RenderBridge.getInstance().documentHeight(mNativeDocument);
                        int width = RenderBridge.getInstance().documentWidth(mNativeDocument);
                        setSize(width, height);
                        invalidate();
                    }else{
                        Log.e("weex", "Weex Request Layout execute none " + hashCode());
                    }
                }
                break;
                case MSG_RENDER_ACTION_EVENT:{
                    if(mNativeDocument != 0){
                        int x = msg.arg1;
                        int y = msg.arg2;
                        int type = (int) msg.obj;
                        final String ref = RenderBridge.getInstance().actionEvent(mNativeDocument, 0, x, y);
                        if(!TextUtils.isEmpty(ref)){
                           onEvent(type, ref);
                        }
                    }
                }
                break;
                case MSG_RENDER_REFRESH_FONT:{
                    if(mNativeDocument != 0){
                        String fontFamily = (String) msg.obj;
                        RenderBridge.getInstance().actionRefreshFont(mNativeDocument, fontFamily);
                        requestLayout();
                    }
                }
                break;
                default:
                    break;
            }
            return true;
    }

    public String hitTest(int type, int x, int y){
        if(mNativeDocument != 0) {
            return RenderBridge.getInstance().actionEvent(mNativeDocument, type, x, y);
        }
        return null;
    }

    private void onEvent(final int eventType, final String ref) {
        if(mDocumentAdapter != null && mDocumentAdapter.getEventListener() != null
                && eventType == OnEventListener.EVENT_TYPE_CLICK){
            final int x = RenderBridge.getInstance().getBlockLayout(mNativeDocument, ref, 0);
            final int y = RenderBridge.getInstance().getBlockLayout(mNativeDocument, ref, 1);
            final int width = RenderBridge.getInstance().getBlockLayout(mNativeDocument, ref, 2);
            final int height = RenderBridge.getInstance().getBlockLayout(mNativeDocument, ref, 3);
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                    if(mDocumentAdapter != null && mDocumentAdapter.getEventListener() != null) {
                        mDocumentAdapter.getEventListener().onClick(ref, x, y, width, height);
                    }
                }
            });
        }
    }


    public void setDocumentAdapter(DocumentAdapter documentAdapter) {
        this.mDocumentAdapter = documentAdapter;
    }


    public int getDocumentHeight() {
        return documentHeight;
    }

    public int getDocumentWidth() {
        return documentWidth;
    }

    public Object getInstance() {
        return instance;
    }

    public void setInstance(Object instance) {
        this.instance = instance;
    }

}
