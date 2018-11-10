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

import android.content.Context;
import android.support.annotation.NonNull;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import com.taobao.weex.WXSDKInstance;
import com.taobao.weex.WXSDKManager;
import com.taobao.weex.common.Constants;
import com.taobao.weex.render.event.OnFrameEventListener;
import com.taobao.weex.render.event.OnFrameImageListener;
import com.taobao.weex.render.event.RenderFrameAdapter;
import com.taobao.weex.render.frame.RenderFrame;
import com.taobao.weex.render.frame.RenderFrameView;
import com.taobao.weex.render.image.FrameImage;
import com.taobao.weex.ui.ComponentCreator;
import com.taobao.weex.ui.action.BasicComponentData;
import com.taobao.weex.ui.action.GraphicPosition;
import com.taobao.weex.ui.action.GraphicSize;
import com.taobao.weex.ui.component.WXA;
import com.taobao.weex.ui.component.WXBasicComponentType;
import com.taobao.weex.ui.component.WXComponent;
import com.taobao.weex.ui.component.WXImage;
import com.taobao.weex.ui.component.WXVContainer;
import com.taobao.weex.utils.WXDataStructureUtil;

import java.lang.reflect.InvocationTargetException;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * Created by furture on 2018/8/7.
 */

public class WXRenderFrameComponent extends WXVContainer<ViewGroup> implements OnFrameImageListener, OnFrameEventListener {

    public static final String RENDER_FRAME_COMPONENT = "segment";

    private RenderFrame renderFrame;
    private RenderFrameAdapter renderFrameAdapter;
    public RenderFrameView renderFrameView;
    private WXRenderFrameHelper renderFrameHelper;
    private WXFrameMeasurement frameSizeChangedListener;
    private boolean documentShouldInited;

    public void updateWatchComponentStatus() {
        if(renderFrameHelper != null){
            renderFrameHelper.updateChildWatchEvents();
        }
    }

    public static class Ceator implements ComponentCreator {
        public WXComponent createInstance(WXSDKInstance instance, WXVContainer parent, BasicComponentData basicComponentData) throws IllegalAccessException, InvocationTargetException, InstantiationException {
            return new WXRenderFrameComponent(instance, parent, basicComponentData);
        }
    }
    public WXRenderFrameComponent(WXSDKInstance instance, WXVContainer parent, String instanceId, boolean isLazy, BasicComponentData basicComponentData) {
       this(instance, parent, basicComponentData);
    }

    public WXRenderFrameComponent(WXSDKInstance instance, WXVContainer parent, boolean lazy, BasicComponentData basicComponentData) {
        this(instance, parent, basicComponentData);
    }

    public WXRenderFrameComponent(WXSDKInstance instance, WXVContainer parent, BasicComponentData basicComponentData) {
        super(instance, parent, basicComponentData);
        lazy(true);
        instance.setHasRenderFrameComponent(true);
        actionCreateBody();
        setContentBoxMeasurement(frameSizeChangedListener);
    }


    @Override
    protected FrameLayout initComponentHostView(@NonNull Context context) {
        renderFrameView = new RenderFrameView(context);
        renderFrameView.setRenderFrame(renderFrame);
        FrameLayout frameLayout = new FrameLayout(context);
        frameLayout.addView(renderFrameView, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
       return frameLayout;
    }

    @Override
    public void addChild(WXComponent child, int index) {
        if(child != null){
            child.lazy(true);
        }
        super.addChild(child, index);
    }

    private void actionCreateBody(){
        renderFrame = new RenderFrame(getContext());
        renderFrame.setInstance(getInstance());
        renderFrameAdapter = new RenderFrameAdapter();
        renderFrameAdapter.setRequireFrameSizeChangedOnMainThread(false);
        frameSizeChangedListener = new WXFrameMeasurement(this);
        renderFrameAdapter.setOnFrameSizeChangedListener(frameSizeChangedListener);
        renderFrameAdapter.setOnImgLoadListener(this);
        renderFrameAdapter.setOnFrameEventListener(this);
        renderFrame.setFrameAdapter(renderFrameAdapter);
        renderFrameHelper = new WXRenderFrameHelper(this);


        String ref = getRef();
        Map<String, String> styles =  toMap(getStyles());
        Map<String, String> attrs = toMap(getAttrs());
        Collection<String> events = getEvents();
        actionCreateBody(ref, styles, attrs, events);
    }

    public void actionCreateBody(String ref, Map<String, String> style, Map<String, String> attrs, Collection<String> events){
        renderFrame.actionCreateBody(ref, style, attrs, events);
    }

    public void actionAddElement(String ref, String componentType, String parentRef, int index, Map<String, String> style, Map<String, String> attrs, Collection<String> events){
        renderFrame.actionAddElement(ref, componentType, parentRef, index, style, attrs, events);
    }

    public void actionUpdateStyles(String ref, Map<String, Object> styles){
        renderFrame.actionUpdateStyles(ref, toMap(styles));
    }

    public void actionUpdateAttrs(String ref, Map<String, String> attrs){
        renderFrame.actionUpdateAttrs(ref, attrs);
    }

    public void actionAddEvent(String ref, Object event){
        renderFrame.actionAddEvent(ref, event.toString());

    }

    public void actionRemoveEvent(String ref, Object event) {
        renderFrame.actionRemoveEvent(ref, event.toString());
    }


    public void actionMoveElement(String ref, String parentRef, int index){
        renderFrame.actionMoveElement(ref, parentRef, index);
    }

    public void actionRemoveElement(String ref) {
        renderFrame.actionRemoveElement(ref);
    }



    @Override
    public void updateStyles(WXComponent component) {
        if(component != this){
            renderFrame.actionUpdateStyles(getRef(),toMap(component.getStyles()));
        }
    }
    @Override
    public void updateAttrs(WXComponent component) {
        if(component != this){
            renderFrame.actionUpdateAttrs(getRef(),toMap(component.getAttrs()));
        }
    }

    @Override
    public void updateStyles(Map<String, Object> styles) {
        renderFrame.actionUpdateStyles(getRef(),toMap(styles));
    }

    @Override
    public void updateAttrs(Map<String, Object> attrs) {
        renderFrame.actionUpdateAttrs(getRef(),toMap(attrs));
    }

    @Override
    public void onActivityResume() {
        super.onActivityResume();
    }

    @Override
    public void updateProperties(Map<String, Object> props) {}

    protected boolean setProperty(String key, Object param) {
        switch (key) {
            case Constants.Name.BACKGROUND_COLOR:
                return true;
            case Constants.Name.OPACITY:
                return true;
            case Constants.Name.BORDER_RADIUS:
            case Constants.Name.BORDER_TOP_LEFT_RADIUS:
            case Constants.Name.BORDER_TOP_RIGHT_RADIUS:
            case Constants.Name.BORDER_BOTTOM_RIGHT_RADIUS:
            case Constants.Name.BORDER_BOTTOM_LEFT_RADIUS:
                return true;
            case Constants.Name.BORDER_STYLE:
            case Constants.Name.BORDER_RIGHT_STYLE:
            case Constants.Name.BORDER_BOTTOM_STYLE:
            case Constants.Name.BORDER_LEFT_STYLE:
            case Constants.Name.BORDER_TOP_STYLE:
                return true;
            case Constants.Name.BORDER_COLOR:
            case Constants.Name.BORDER_TOP_COLOR:
            case Constants.Name.BORDER_RIGHT_COLOR:
            case Constants.Name.BORDER_BOTTOM_COLOR:
            case Constants.Name.BORDER_LEFT_COLOR:
                return true;
            case Constants.Name.BOX_SHADOW:
                return true;
            default:break;
        }
        return super.setProperty(key, param);
    }

    @Override
    public void setBorderRadius(String key, float borderRadius) {}

    @Override
    public void setBorderStyle(String key, String borderStyle) {}

    @Override
    public void setBorderWidth(String key, float borderWidth) {}




    @Override
    public void destroy() {
        synchronized (this){
            super.destroy();
        }
        if(renderFrameView != null){
            renderFrameView.destroy();
        }
    }

    private Map<String, String> toMap(Map<String, Object> style){
        if(style == null){
            return null;
        }
        Map<String, String> maps = new HashMap<>();
        Set<Map.Entry<String,Object>> entries = style.entrySet();
        for(Map.Entry<String,Object> entry: entries){
            if(entry.getValue() == null){
                continue;
            }
            maps.put(entry.getKey(), entry.getValue().toString());
        }
        return maps;
    }



    public static WXRenderFrameComponent getRenderFrameComponent(WXComponent parent){
        while (parent != null){
            if(RENDER_FRAME_COMPONENT.equals(parent.getComponentType())){
                return (WXRenderFrameComponent) parent;
            }
            if(WXBasicComponentType.CELL.equals(parent.getComponentType())){
                return null;
            }
            if(WXBasicComponentType.LIST.equals(parent.getComponentType())){
                return null;
            }
            parent = parent.getParent();
        }
        return null;
    }

    @Override
    public void notifyAppearStateChange(String wxEventType, String direction) {
        if(renderFrameHelper != null){
            renderFrameHelper.notifyAppearStateChange(wxEventType, direction);
        }
    }

    @Override
    public void setDemission(GraphicSize size, GraphicPosition position) {
        super.setDemission(size, position);
        if(getHostView() == null){
            if(documentShouldInited){
                new InitRenderFrameViewAction(this).run();
            }
        }
    }

    @Override
    public void onLoadImage(FrameImage frameImage) {
        if(frameImage.isPlaceholder()){
            return;
        }
        WXComponent component = WXSDKManager.getInstance().getWXRenderManager().getWXComponent(getInstanceId(), frameImage.getRef());
        if(component == null || !(component instanceof WXImage)){
            return;
        }
        WXImage image = (WXImage) component;
        int width = 0;
        int height =0;
        boolean result = false;
        if(frameImage.getBitmap() != null){
            width = frameImage.getBitmap().getWidth();
            height = frameImage.getBitmap().getHeight();
            result = true;
        }
        image.fireOnLoadEvent(width, height, result);
    }

    @Override
    public void onClick(String ref, int x, int y, int width, int height) {
        if(getHostView() == null || isDestoryed()){
            return;
        }
        WXComponent component = WXSDKManager.getInstance().getWXRenderManager().getWXComponent(getInstanceId(), ref);
        if(component == null){
            return;
        }
        int[] location = new int[2];
        getHostView().getLocationOnScreen(location);
        x += location[0];
        y += location[1];
        Map<String, Object> param = WXDataStructureUtil.newHashMapWithExpectedSize(1);
        Map<String, Object> position = WXDataStructureUtil.newHashMapWithExpectedSize(4);
        position.put("x", x);
        position.put("y",y);
        position.put("width", width);
        position.put("height", height);
        param.put(Constants.Name.POSITION, position);
        component.fireEvent(Constants.Event.CLICK, param);
        if(component instanceof WXA){
            ((WXA)(component)).onHostViewClick();
        }
    }

    public RenderFrame getRenderFrameComponent() {
        return renderFrame;
    }

    public boolean isDocumentShouldInited() {
        return documentShouldInited;
    }

    public void setDocumentShouldInited(boolean documentShouldInited) {
        this.documentShouldInited = documentShouldInited;
    }
}
