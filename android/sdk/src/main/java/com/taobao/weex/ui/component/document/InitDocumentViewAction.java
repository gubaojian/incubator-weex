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
package com.taobao.weex.ui.component.document;

import com.taobao.weex.WXSDKInstance;
import com.taobao.weex.ui.action.BasicGraphicAction;
import com.taobao.weex.ui.component.WXVContainer;

/**
 * Created by furture on 2018/9/4.
 */

public class InitDocumentViewAction extends BasicGraphicAction {

    private WXDocumentComponent documentComponent;


    public InitDocumentViewAction(WXDocumentComponent documentComponent) {
        super(documentComponent.getInstance(), documentComponent.getRef());
        this.documentComponent = documentComponent;
    }

    @Override
    public void executeAction() {
        if(documentComponent.getHostView() != null){
            return;
        }
        documentComponent.lazy(false);
        WXVContainer parent = documentComponent.getParent();
        if(parent != null){
            int index = parent.indexOf(documentComponent);
            parent.createChildViewAt(index);
        }
        documentComponent.applyLayoutAndEvent(documentComponent);
        documentComponent.bindData(documentComponent);
    }
}
