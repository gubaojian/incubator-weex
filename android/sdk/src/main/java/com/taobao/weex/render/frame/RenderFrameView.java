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
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.support.v4.view.ViewCompat;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.TextureView;
import android.view.View;

import com.taobao.weex.render.accessibility.RenderFrameAccessibilityHelper;
import com.taobao.weex.render.fling.FlingControl;
import com.taobao.weex.render.manager.RenderStats;
import com.taobao.weex.render.event.OnFrameEventListener;


/**
 * Created by furture on 2018/7/23.
 */

public class RenderFrameView extends TextureView implements  TextureView.SurfaceTextureListener, View.OnClickListener{


    private RenderFrame renderFrame;
    private boolean hasAttachToWindow;
    private MotionEvent lastEvent;
    private RenderFrameAccessibilityHelper renderFrameAccessibilityHelper;
    private SurfaceTextureHolder surfaceTextureHolder;
    private RenderFrameRender renderFrameRender;

    public RenderFrameView(Context context) {
        super(context);
        initView();
    }

    public RenderFrameView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
    }

    public RenderFrameView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initView();
    }

    public RenderFrameView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        initView();
    }


    private void initView() {
        setOpaque(false);
        setSurfaceTextureListener(this);
        setOnClickListener(this);
        if(getLayerType() != View.LAYER_TYPE_HARDWARE) {
            setLayerType(View.LAYER_TYPE_HARDWARE, null);
        }
        try{
            renderFrameAccessibilityHelper = new RenderFrameAccessibilityHelper(this);
            ViewCompat.setAccessibilityDelegate(this, renderFrameAccessibilityHelper);
        }catch (Exception e){}
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        lastEvent = event;
        return super.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return renderFrameAccessibilityHelper.dispatchKeyEvent(event) ||super.dispatchKeyEvent(event);
    }

    @Override
    protected boolean dispatchHoverEvent(MotionEvent event) {
        lastEvent = event;
        return renderFrameAccessibilityHelper.dispatchHoverEvent(event) || super.dispatchHoverEvent(event);
    }

    @Override
    public void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        renderFrameAccessibilityHelper.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
    }

    @Override
    public void onSurfaceTextureAvailable(final SurfaceTexture surfaceTexture, final int width, final int height) {
        if(renderFrame == null){
            return;
        }
        renderFrame.setPause(false);
        surfaceTextureHolder = new SurfaceTextureHolder(surfaceTexture, width, height);
        renderFrameRender = new RenderFrameRender(renderFrame, surfaceTextureHolder);
        surfaceTextureHolder.setRenderFrameRender(renderFrameRender);
        surfaceTextureHolder.getRenderFrameRender().attachEGLSurfaceTexture(surfaceTextureHolder);
    }



    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int width, int height) {
        if(renderFrame != null){
            surfaceTextureHolder.setSurfaceTexture(surfaceTexture, width, height);
            surfaceTextureHolder.getRenderFrameRender().resizeSurfaceTexture(surfaceTextureHolder);
        }
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
        if(surfaceTextureHolder != null){
            renderFrameRender.getRenderFrame().setPause(true);
            /**
             * first set destory true, to notify GPUThread task is invalid, should not run
             * then use synchronized to wait valid task complete.
             * */
            surfaceTextureHolder.setDestory(true);
            synchronized (renderFrameRender){
                surfaceTextureHolder.setDestory(true);
            }
            surfaceTextureHolder.getRenderFrameRender().dettachSurfaceTexture(surfaceTextureHolder);
            RenderStats.countPeriodDettachNum(getContext());
        }
        surfaceTextureHolder = null;
        renderFrameRender = null;
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {}


    @Override
    protected void onAttachedToWindow() {
        hasAttachToWindow = true;
        super.onAttachedToWindow();
        FlingControl.setParentMaxFling(this);
    }

    @Override
    protected void onDetachedFromWindow() {
        hasAttachToWindow = false;
        super.onDetachedFromWindow();
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
    }



    public boolean isHasAttachToWindow() {
        return hasAttachToWindow;
    }

    @Override
    public void onClick(View v) {
        if(renderFrame == null){
            return;
        }
        int[] location = new int[2];
        v.getLocationOnScreen(location);
        float x = lastEvent.getRawX()  - location[0];
        float y = lastEvent.getRawY() - location[1];
        renderFrame.actionHitTestEvent(OnFrameEventListener.FRAME_EVENT_TYPE_CLICK, x, y);
    }


    public boolean hasRenderFrame(){
        return renderFrame != null;
    }


    public RenderFrame getRenderFrame(){
        if(renderFrame == null){
            renderFrame = new RenderFrame(getContext());
        }
        return renderFrame;
    }

    public void setRenderFrame(RenderFrame renderFrame) {
        this.renderFrame = renderFrame;
        if(hasAttachToWindow){
            throw new RuntimeException("setRenderFrame should be be remove first");
        }
    }



    public void destroy(){
        if(renderFrame != null){
            renderFrame.destory();
        }
    }



    @Override
    protected void finalize() throws Throwable {
        destroy();
        super.finalize();
    }
}
