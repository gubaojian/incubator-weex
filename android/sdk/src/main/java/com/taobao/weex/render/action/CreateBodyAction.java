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

import com.taobao.weex.render.bridge.RenderBridge;
import com.taobao.weex.render.log.RenderLog;
import com.taobao.weex.render.frame.RenderFrame;

import java.util.Collection;
import java.util.Map;

/**
 * Created by furture on 2018/8/8.
 */

public class CreateBodyAction extends  Action {

    private String ref;
    private Map<String, String> style;
    private Map<String, String> attrs;
    private Collection<String> events;

    public CreateBodyAction(RenderFrame documentView, String ref, Map<String, String> style, Map<String, String> attrs, Collection<String> events) {
        super(documentView);
        this.ref = ref;
        this.style = style;
        this.attrs = attrs;
        this.events = events;
    }
    @Override
    protected void execute() {
        if(getRenderFrame().getNativeFrame() != 0){
            RenderBridge.getInstance().destroyFrame(getRenderFrame().getNativeFrame());
        }

        RenderLog.actionCreateBody(getRenderFrame(), ref, style, attrs, events);
        getRenderFrame().setNativeFrame(RenderBridge.getInstance().createFrame(getRenderFrame().getFrameKey()));
        getRenderFrame().getFrameImageMap().clear();
        RenderBridge.getInstance().createBody(getRenderFrame().getNativeFrame(), ref, style, attrs, events);
        getRenderFrame().requestLayout();
    }
}