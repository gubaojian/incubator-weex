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
package com.taobao.weex.ui.component.frame;

import android.util.Log;

import com.alibaba.fastjson.JSON;
import com.taobao.weex.WXSDKManager;
import com.taobao.weex.bridge.WXBridgeManager;
import com.taobao.weex.common.Constants;
import com.taobao.weex.layout.ContentBoxMeasurement;
import com.taobao.weex.layout.MeasureMode;
import com.taobao.weex.render.event.OnFrameSizeChangedListener;
import com.taobao.weex.render.frame.RenderFrame;
import com.taobao.weex.utils.WXUtils;
import com.taobao.weex.utils.WXViewUtils;

import java.util.HashMap;
import java.util.Map;

/**
 * Created by furture on 2018/8/23.
 */

public class WXFrameMeasurement extends ContentBoxMeasurement implements OnFrameSizeChangedListener, Runnable {


    private WXRenderFrameComponent renderFrameComponent;
    private float lastComputedWidth;
    private float lastComputedHeight;
    private float maxHeight;
    private float maxWidth;

    public WXFrameMeasurement(WXRenderFrameComponent renderFrameComponent) {
        this.renderFrameComponent = renderFrameComponent;
        this.maxHeight = WXViewUtils.getScreenHeight(renderFrameComponent.getContext())*2;
        this.maxWidth = maxHeight;
    }



    @Override
    public void measureInternal(float width, float height, int widthMeasureMode, int heightMeasureMode) {
        if(renderFrameComponent.getRenderFrameComponent() == null){
            return;
        }
        if(renderFrameComponent.getRenderFrameComponent().getFrameWidth() <= 0
                || renderFrameComponent.getRenderFrameComponent().getFrameHeight() <= 0){
            return;
        }
        if(widthMeasureMode != MeasureMode.EXACTLY){
            mMeasureWidth = renderFrameComponent.getRenderFrameComponent().getFrameWidth();
            if(mMeasureWidth > maxWidth){
                mMeasureWidth  = maxWidth;
            }
        }else{
            mMeasureWidth = width;
        }
        if(heightMeasureMode != MeasureMode.EXACTLY){
            mMeasureHeight = renderFrameComponent.getRenderFrameComponent().getFrameHeight();
            if(mMeasureHeight > maxHeight){
                mMeasureHeight  = maxHeight;
            }
        }else{
            mMeasureHeight= height;
        }
    }

    @Override
    public void layoutBefore() {

    }

    @Override
    public void layoutAfter(float computedWidth, float computedHeight) {
        if(computedWidth <= 0 || computedHeight <= 0 || Float.isNaN(computedHeight)){
            return;
        }
        if(computedWidth >= maxWidth){
            return;
        }
        if(computedHeight >= maxHeight){
            return;
        }

        componentShouldInit();
        if(renderFrameComponent.getRenderFrameComponent() == null){
            return;
        }
        if(lastComputedWidth != computedWidth || lastComputedHeight != computedHeight){
            lastComputedWidth = computedWidth;
            lastComputedHeight = computedHeight;

            Map<String,Object> map = null;
            if(shouldUpdateHeight(computedHeight)){
                int pixelHeight = (int) computedHeight;
                if(pixelHeight != renderFrameComponent.getRenderFrameComponent().getFrameHeight()){
                    if(map == null){
                        map = new HashMap<>();
                    }
                    map.put(Constants.Name.HEIGHT, computedHeight*750/ WXViewUtils.getScreenWidth());
                }
            }
            if(shouldUpdateWidth(computedWidth)){
                int pixelWidth = (int) computedWidth;
                if(pixelWidth != renderFrameComponent.getRenderFrameComponent().getFrameWidth()){
                    if(map == null){
                        map = new HashMap<>();
                    }
                    map.put(Constants.Name.WIDTH, computedWidth*750/WXViewUtils.getScreenWidth());
                }
            }
            if(map != null && map.size() > 0){
                renderFrameComponent.updateStyles(map);
                Log.e("Weex", "Weex layout measure update styles " + JSON.toJSONString(map)
                + "  " + renderFrameComponent.getRenderFrameComponent().getFrameWidth());
            }
        }
    }

    private boolean shouldUpdateHeight(float computedHeight){
        if(computedHeight <= 0){
            return false;
        }
        if(renderFrameComponent.getStyles().get(Constants.Name.HEIGHT) != null){
            float styleHeight = WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(renderFrameComponent.getStyles().get(Constants.Name.HEIGHT),
                    renderFrameComponent.getViewPortWidth()), renderFrameComponent.getViewPortWidth());
            if(Math.abs(styleHeight - (int)computedHeight) <= 0.1){
                return false;
            }
        }
        return true;
    }

    private boolean shouldUpdateWidth(float computedWidth){
        if(computedWidth <= 0){
            return false;
        }
        if(renderFrameComponent.getStyles().get(Constants.Name.WIDTH) != null){
            float styleWidth = WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(renderFrameComponent.getStyles().get(Constants.Name.WIDTH),
                    renderFrameComponent.getViewPortWidth()), renderFrameComponent.getViewPortWidth());
            if(Math.abs(styleWidth - (int)computedWidth) <= 0.1){
                return false;
            }
        }
        return true;
    }


    @Override
    public void onSizeChanged(final RenderFrame renderFrame, final int width, final int height) {
        if(renderFrame.getFrameHeight() != height || renderFrame.getFrameWidth() != width){
            return;
        }
        synchronized (renderFrameComponent){
            if(renderFrameComponent.isDestoryed()){
                return;
            }
            componentShouldInit();
            if(renderFrameComponent.getStyles().containsKey(Constants.Name.WIDTH) && renderFrameComponent.getStyles().containsKey(Constants.Name.HEIGHT)){
                return;
            }
            WXBridgeManager.getInstance().markDirty(renderFrameComponent.getInstanceId(), renderFrameComponent.getRef(), true);
        }

        WXSDKManager.getInstance().getWXBridgeManager().removeCallback(this);
        WXSDKManager.getInstance().getWXBridgeManager().postAtFrontOfQueue(this);
    }

    @Override
    public void run() {
        if(renderFrameComponent.isDestoryed()){
            return;
        }
        WXBridgeManager.getInstance().markDirty(renderFrameComponent.getInstanceId(), renderFrameComponent.getRef(), true);
    }

    private void componentShouldInit(){
        if(!renderFrameComponent.isDocumentShouldInited()){
            if(renderFrameComponent.getLayoutSize().getWidth() > 0 && renderFrameComponent.getLayoutSize().getHeight() > 0){
                WXSDKManager.getInstance().getWXRenderManager().postGraphicAction(renderFrameComponent.getInstanceId(), new InitRenderFrameViewAction(renderFrameComponent));
            }
            renderFrameComponent.setDocumentShouldInited(true);
        }
    }
}
