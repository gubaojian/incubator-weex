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
package com.taobao.weex.render.action;

import android.text.TextUtils;

import com.taobao.weex.render.bridge.RenderBridge;
import com.taobao.weex.render.event.OnFrameEventListener;
import com.taobao.weex.render.event.RenderFrameAdapter;
import com.taobao.weex.render.manager.RenderManager;
import com.taobao.weex.render.frame.RenderFrame;

public class HitTestEventAction extends  Action {

    /**
     * event type and x y location
     * */
    private int mType;
    private int mX;
    private int mY;

    public HitTestEventAction(RenderFrame renderFrame, int type, int x, int y) {
        super(renderFrame);
        this.mType = type;
        this.mX = x;
        this.mY = y;
    }

    @Override
    protected void execute() {
        long mNativeFrame = getRenderFrame().getNativeFrame();
        final String ref = RenderBridge.getInstance().actionHitTestEvent(getRenderFrame().getNativeFrame(), mType, mX, mY);
        if(TextUtils.isEmpty(ref)){
            return;
        }
        final RenderFrameAdapter renderFrameAdapter = getRenderFrame().getFrameAdapter();
        if(renderFrameAdapter == null){
            return;
        }

        if(mType == OnFrameEventListener.FRAME_EVENT_TYPE_CLICK
                && renderFrameAdapter.getFrameEventListener() != null){
            final int x = RenderBridge.getInstance().getBlockLayout(mNativeFrame, ref, 0);
            final int y = RenderBridge.getInstance().getBlockLayout(mNativeFrame, ref, 1);
            final int width = RenderBridge.getInstance().getBlockLayout(mNativeFrame, ref, 2);
            final int height = RenderBridge.getInstance().getBlockLayout(mNativeFrame, ref, 3);

            RenderManager.getInstance().getUiHandler().post(new Runnable() {
                @Override
                public void run() {
                    if(getRenderFrame().isDestroy()){
                        return;
                    }
                    renderFrameAdapter.getFrameEventListener().onClick(ref, x, y, width, height);
                }
            });
        }

    }
}
