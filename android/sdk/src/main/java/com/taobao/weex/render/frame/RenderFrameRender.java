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

import android.os.Handler;
import com.taobao.weex.render.egl.AttachEGLAction;
import com.taobao.weex.render.egl.DettachEGLAction;
import com.taobao.weex.render.egl.InvalidateEGLAction;
import com.taobao.weex.render.egl.ResizeEGLAction;
import com.taobao.weex.render.manager.RenderStats;
import com.taobao.weex.render.threads.GpuThread;


public class RenderFrameRender{

    private RenderFrame mRenderFrame;
    private long mNativeFrameRender;
    private Handler gpuHandler;
    private SurfaceTextureHolder surfaceTextureHolder;
    private GpuThread gpuThread;


    public RenderFrameRender(RenderFrame renderFrame, SurfaceTextureHolder surfaceTextureHolder) {
        this.mRenderFrame = renderFrame;
        this.gpuThread = GpuThread.newGpuThread(this);
        this.gpuHandler = new Handler(gpuThread.getLooper());
        this.surfaceTextureHolder = surfaceTextureHolder;
    }



    public void attachEGLSurfaceTexture(SurfaceTextureHolder surfaceTextureHolder){
        AttachEGLAction attachOpenGLAction = new AttachEGLAction(mRenderFrame, this);
        gpuHandler.postAtFrontOfQueue(attachOpenGLAction);
        attachOpenGLAction.waitComplete();

    }

    public void resizeSurfaceTexture(SurfaceTextureHolder surfaceTextureHolder){
        ResizeEGLAction resizeEGLAction = new ResizeEGLAction(mRenderFrame, this);
        gpuHandler.post(resizeEGLAction);
    }

    public void dettachSurfaceTexture(SurfaceTextureHolder surfaceTextureHolder){
        DettachEGLAction dettachEGLAction = new DettachEGLAction(mRenderFrame, this);
        gpuHandler.post(dettachEGLAction);
        dettachEGLAction.waitComplete();
    }

    public void invalidate() {
        if(mNativeFrameRender == 0){
            return;
        }
        InvalidateEGLAction invalidateEGLAction = new InvalidateEGLAction(mRenderFrame, this);
        gpuHandler.post(invalidateEGLAction);
    }


    public RenderFrame getRenderFrame() {
        return mRenderFrame;
    }

    public void setNativeFrameRender(long nativeFrameRender){
        this.mNativeFrameRender =  nativeFrameRender;
    }
    public long getNativeFrameRender() {
        return mNativeFrameRender;
    }


    public void destroy(){
        GpuThread.quitGpuThread(gpuThread);
    }


    @Override
    protected void finalize() throws Throwable {
        if(mNativeFrameRender != 0){
            destroy();
        }
        super.finalize();
    }

    public SurfaceTextureHolder getSurfaceTextureHolder() {
        return surfaceTextureHolder;
    }
}
