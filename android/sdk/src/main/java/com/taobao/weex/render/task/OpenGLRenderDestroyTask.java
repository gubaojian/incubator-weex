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
package com.taobao.weex.render.task;

import com.taobao.weex.render.view.DocumentView;
import com.taobao.weex.render.view.SurfaceTextureHolder;

/**
 * Created by furture on 2018/8/14.
 */

public class OpenGLRenderDestroyTask extends GLTask {

    private SurfaceTextureHolder surfaceTextureHolder;
    private int token;

    public OpenGLRenderDestroyTask(DocumentView documentView, final SurfaceTextureHolder surfaceTextureHolder) {
        super(documentView);
        this.surfaceTextureHolder = surfaceTextureHolder;
        this.token = documentView.getRenderStage().get();
    }

    @Override
    public void run() {
        OpenGLRender openGLRender = surfaceTextureHolder.getOpenGLRender();
        if(openGLRender != null){
            openGLRender.setWillDestory(true);
            openGLRender.destroy();
            surfaceTextureHolder.setOpenGLRender(null);
        }
    }
}
