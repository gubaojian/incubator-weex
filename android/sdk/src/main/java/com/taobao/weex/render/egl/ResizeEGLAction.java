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

public class ResizeEGLAction extends EGLAction {

    private SurfaceTextureHolder surfaceTextureHolder;

    public ResizeEGLAction(RenderFrame renderFrame, RenderFrameRender renderFrameRender) {
        super(renderFrame, renderFrameRender);
        this.surfaceTextureHolder = renderFrameRender.getSurfaceTextureHolder();
    }



    @Override
    protected void execute() {
        if(surfaceTextureHolder.isDestory()){
            return;
        }

        long mNativeFrameRender = getRenderFrameRender().getNativeFrameRender();
        RenderBridge.getInstance().frameRenderSizeChanged(mNativeFrameRender, surfaceTextureHolder.getWidth(), surfaceTextureHolder.getHeight());
        if(RenderLog.isDebugLogEnabled()){
            RenderLog.debug("ResizeEGLAction " + mNativeFrameRender);
        }
        if(surfaceTextureHolder.isDestory()){
            return;
        }

        //repaint first times
        RenderBridge.getInstance().frameRenderInvalidate(mNativeFrameRender);
        if(surfaceTextureHolder.isDestory()){
            return;
        }
        RenderBridge.getInstance().frameRenderSwap(mNativeFrameRender);
        if(surfaceTextureHolder.isDestory()){
            return;
        }




        //repaint second times
        RenderBridge.getInstance().frameRenderInvalidate(mNativeFrameRender);
        if(surfaceTextureHolder.isDestory()){
            return;
        }
        RenderBridge.getInstance().frameRenderSwap(mNativeFrameRender);
        if(surfaceTextureHolder.isDestory()){
            return;
        }

        //repaint full frame
        getRenderFrame().requestFrame();
    }
}
