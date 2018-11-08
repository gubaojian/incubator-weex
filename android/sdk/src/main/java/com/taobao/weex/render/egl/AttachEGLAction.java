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
package com.taobao.weex.render.egl;


import com.taobao.weex.render.bridge.RenderBridge;
import com.taobao.weex.render.frame.RenderFrame;
import com.taobao.weex.render.frame.RenderFrameRender;
import com.taobao.weex.render.frame.SurfaceTextureHolder;
import com.taobao.weex.render.log.RenderLog;
import com.taobao.weex.render.manager.RenderStats;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class AttachEGLAction extends EGLAction {

    private SurfaceTextureHolder surfaceTextureHolder;
    private CountDownLatch countDownLatch;

    public AttachEGLAction(RenderFrame renderFrame, RenderFrameRender renderFrameRender) {
        super(renderFrame, renderFrameRender);
        surfaceTextureHolder = renderFrameRender.getSurfaceTextureHolder();
        RenderStats.getCurrentWaitEglTaskNum().incrementAndGet();
        if(RenderStats.getCountDettachNum() > RenderStats.MAX_DETTACH_NUM_ON_SECOND){
            countDownLatch = new CountDownLatch(1);
        }
    }

    @Override
    protected void runFrameRender() {
        try{
            super.runFrameRender();
        }finally {
            RenderStats.getElgNum().incrementAndGet();
            RenderStats.getCurrentWaitEglTaskNum().decrementAndGet();
            if(countDownLatch != null){
                countDownLatch.countDown();
            }
        }
    }

    @Override
    protected void execute() {
        if(surfaceTextureHolder.isDestory()){
            return;
        }
        long mNativeFrameRender = RenderBridge.getInstance().frameRenderAttach(surfaceTextureHolder.createSurface(), surfaceTextureHolder.getWidth(),
                surfaceTextureHolder.getHeight());
        if(RenderLog.isDebugLogEnabled()){
            RenderLog.debug("AttachEGLAction " + mNativeFrameRender);
        }
        getRenderFrameRender().setNativeFrameRender(mNativeFrameRender);


        if(surfaceTextureHolder.isDestory()){
            return;
        }
        RenderBridge.getInstance().frameRenderClearBuffer(mNativeFrameRender);


        if(surfaceTextureHolder.isDestory()){
            return;
        }
        RenderBridge.getInstance().frameRenderSwap(mNativeFrameRender);



        if(surfaceTextureHolder.isDestory()){
            return;
        }
        getRenderFrame().setFrameRender(getRenderFrameRender());
        getRenderFrame().requestFrame();
    }

    public void waitComplete(){
        if(countDownLatch != null){
            try {
                countDownLatch.await(300, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
