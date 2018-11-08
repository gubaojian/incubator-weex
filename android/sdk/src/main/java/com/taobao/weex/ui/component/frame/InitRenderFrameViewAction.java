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

import com.taobao.weex.ui.action.BasicGraphicAction;
import com.taobao.weex.ui.component.WXVContainer;

/**
 * Created by furture on 2018/9/4.
 */

public class InitRenderFrameViewAction extends BasicGraphicAction {

    private WXRenderFrameComponent renderFrameComponent;


    public InitRenderFrameViewAction(WXRenderFrameComponent renderFrameComponent) {
        super(renderFrameComponent.getInstance(), renderFrameComponent.getRef());
        this.renderFrameComponent = renderFrameComponent;
    }

    @Override
    public void executeAction() {
        if(renderFrameComponent.getHostView() != null){
            return;
        }
        renderFrameComponent.lazy(false);
        WXVContainer parent = renderFrameComponent.getParent();
        if(parent != null){
            int index = parent.indexOf(renderFrameComponent);
            parent.createChildViewAt(index);
        }
        renderFrameComponent.applyLayoutAndEvent(renderFrameComponent);
        renderFrameComponent.bindData(renderFrameComponent);
    }
}
