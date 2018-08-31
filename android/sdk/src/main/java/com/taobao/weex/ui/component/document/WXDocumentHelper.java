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

import android.util.Log;

import com.taobao.weex.common.Constants;
import com.taobao.weex.ui.component.WXComponent;
import com.taobao.weex.ui.component.WXVContainer;

/**
 * handler appear, disappear, accessibility etc
 * Created by furture on 2018/8/21.
 */
public class WXDocumentHelper {

    private WXDocumentComponent documentComponent;
    private boolean isAppear;


    public WXDocumentHelper(WXDocumentComponent documentComponent) {
        this.documentComponent = documentComponent;
        this.isAppear = false;
    }


    public void updateWatchEvents(){
        if(documentComponent.containsEvent(Constants.Event.APPEAR) || documentComponent.containsEvent(Constants.Event.DISAPPEAR)){
            notifyDocumentNodeAppearEvent(documentComponent, "none");
            return;
        }
        boolean needWatch = needWatch(documentComponent);
        if(!needWatch){
            return;
        }
        documentComponent.getEvents().addEvent(Constants.Event.APPEAR);
        documentComponent.getEvents().addEvent(Constants.Event.DISAPPEAR);
        if(documentComponent.getHostView() != null){
            documentComponent.addEvent(Constants.Event.APPEAR);
            documentComponent.addEvent(Constants.Event.DISAPPEAR);
        }
    }

    public void notifyAppearStateChange(String wxEventType, String direction) {
        if(Constants.Event.APPEAR.equals(wxEventType)){
            isAppear = true;
        }else if(Constants.Event.DISAPPEAR.equals(wxEventType)){
            isAppear = false;
        }
        notifyDocumentNodeAppearEvent(documentComponent, direction);
    }

    private void notifyDocumentNodeAppearEvent(WXComponent component, String direction){
        Log.e("Weex", "weex fire appear notifyDocumentNodeAppearEvent "+ component.getComponentType() + "   " + component.getAttrs().get("value") + "  " + component.getRef());
        if(isAppear){
            component.documentNodeAppearChange(Constants.Event.APPEAR, direction);
        }else{
            component.documentNodeAppearChange(Constants.Event.DISAPPEAR, direction);
        }
        if(component instanceof WXVContainer){
            WXVContainer container = (WXVContainer) component;
            for(int i=0; i<container.getChildCount(); i++){
                notifyDocumentNodeAppearEvent(container.getChild(i), direction);
            }
        }
    }

    private boolean needWatch(WXComponent component){
        if(component.getEvents().contains(Constants.Event.APPEAR)
                || component.getEvents().contains(Constants.Event.DISAPPEAR)){
            return true;
        }
        if(component instanceof WXVContainer){
            WXVContainer container = (WXVContainer) component;
            for(int i=0; i<container.getChildCount(); i++){
                if(needWatch(container.getChild(i))){
                    return true;
                }
            }
        }
        return false;
    }

}
