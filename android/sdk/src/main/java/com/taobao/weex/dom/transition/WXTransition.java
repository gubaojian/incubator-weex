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
package com.taobao.weex.dom.transition;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ArgbEvaluator;
import android.animation.PropertyValuesHolder;
import android.animation.ValueAnimator;
import android.os.Handler;
import android.support.v4.util.ArrayMap;
import android.support.v4.util.ArraySet;
import android.support.v4.view.animation.PathInterpolatorCompat;
import android.text.TextUtils;
import android.util.Log;
import android.view.animation.Interpolator;

import com.taobao.weex.WXEnvironment;
import com.taobao.weex.WXSDKManager;
import com.taobao.weex.common.Constants;
import com.taobao.weex.dom.DOMActionContext;
import com.taobao.weex.dom.WXDomHandler;
import com.taobao.weex.dom.WXDomObject;
import com.taobao.weex.dom.flex.Spacing;
import com.taobao.weex.ui.component.WXComponent;
import com.taobao.weex.utils.SingleFunctionParser;
import com.taobao.weex.utils.WXLogUtils;
import com.taobao.weex.utils.WXResourceUtils;
import com.taobao.weex.utils.WXUtils;
import com.taobao.weex.utils.WXViewUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;

import static com.taobao.weex.common.Constants.TimeFunction.CUBIC_BEZIER;
import static com.taobao.weex.common.Constants.TimeFunction.EASE_IN;
import static com.taobao.weex.common.Constants.TimeFunction.EASE_IN_OUT;
import static com.taobao.weex.common.Constants.TimeFunction.EASE_OUT;
import static com.taobao.weex.common.Constants.TimeFunction.LINEAR;

/**
 *   transition on dom thread
 *   transition-property: height;
 *   transition-duration: .3s;
 *   transition-delay: .05s;
 *   transition-timing-function: ease-in-out;
 *
 *   Created by furture on 2017/10/18.
 */
public class WXTransition {

    public static final  String TRANSITION_PROPERTY = "transitionProperty";
    public static final  String TRANSITION_DURATION = "transitionDuration";
    public static final  String TRANSITION_DELAY = "transitionDelay";
    public static final  String TRANSITION_TIMING_FUNCTION = "transitionTimingFunction";
    public static final  Pattern PROPERTY_SPLIT_PATTERN = Pattern.compile("\\||,");


    public static final Set<String> SUPPORTED_PROPERTIES = new ArraySet<>();
    static {
        SUPPORTED_PROPERTIES.add(Constants.Name.WIDTH);
        SUPPORTED_PROPERTIES.add(Constants.Name.HEIGHT);
        SUPPORTED_PROPERTIES.add(Constants.Name.MARGIN_TOP);
        SUPPORTED_PROPERTIES.add(Constants.Name.MARGIN_BOTTOM);
        SUPPORTED_PROPERTIES.add(Constants.Name.MARGIN_LEFT);
        SUPPORTED_PROPERTIES.add(Constants.Name.MARGIN_RIGHT);
        SUPPORTED_PROPERTIES.add(Constants.Name.OPACITY);
        SUPPORTED_PROPERTIES.add(Constants.Name.BACKGROUND_COLOR);
    }

    private List<String> properties;
    private Interpolator  interpolator;
    private float  duration;
    private float delay;
    private Map<String, Object> targetStyles;
    private WXDomObject domObject;
    private Map<String, Object> pendingUpdates;
    private Handler handler;
    private Runnable animationRunnable;
    private ValueAnimator valueAnimator;

    public WXTransition() {
        this.properties = new ArrayList<>(4);
        this.targetStyles = new ArrayMap<>();
        this.handler = new Handler();
        this.pendingUpdates = new HashMap<>(8);
    }


    public boolean hasTransitionProperty(Map<String, Object> styles){
        for(String property : properties){
            if(styles.containsKey(property)){
                return  true;
            }
        }
        return false;
    }
    public void  startTransition(Map<String, Object> updates){
        for(String property : properties){
            if(updates.containsKey(property)){
                Object targetValue = updates.remove(property);
                pendingUpdates.put(property, targetValue);
            }
        }
        if(animationRunnable != null) {
             handler.removeCallbacks(animationRunnable);
        }
        animationRunnable = new Runnable() {
            @Override
            public void run() {
                doPendingTransitionAnimation();
            }
        };
        handler.postDelayed(animationRunnable, WXUtils.getNumberInt(domObject.getAttrs().get("layoutAnimationDelay"), 15));

    }

    public void doPendingTransitionAnimation(){
        if(pendingUpdates.size() == 0){
            return;
        }
        if(valueAnimator != null){
            valueAnimator.cancel();
        }
        Map<String, Object> updates = pendingUpdates;
        PropertyValuesHolder[] holders = new PropertyValuesHolder[updates.size()];
        int index = 0;
        for(String property : properties){
            if(updates.containsKey(property)){
                Object targetValue = updates.remove(property);
                targetStyles.put(property, targetValue);
                holders[index] = createPropertyValueHolder(property, targetValue);
                index++;
            }else{
                if(targetStyles.containsKey(property)){
                    domObject.getStyles().put(property, targetStyles.remove(property));
                }
            }
        }
        pendingUpdates.clear();
        doPropertyValuesHolderAnimation(holders);
    }


    private PropertyValuesHolder createPropertyValueHolder(String property, Object value){
        PropertyValuesHolder holder = null;
        switch (property){
            case Constants.Name.WIDTH:{
                holder = PropertyValuesHolder.ofFloat(Constants.Name.WIDTH, domObject.getLayoutWidth(),
                        WXViewUtils.getRealPxByWidth(WXUtils.getFloat(value), domObject.getViewPortWidth()));
            }
            break;
            case Constants.Name.HEIGHT:{
                holder = PropertyValuesHolder.ofFloat(Constants.Name.HEIGHT, domObject.getLayoutHeight(),
                        WXViewUtils.getRealPxByWidth(WXUtils.getFloat(value), domObject.getViewPortWidth()));
            }
            break;
            case Constants.Name.MARGIN_TOP:{
                holder = PropertyValuesHolder.ofFloat(Constants.Name.MARGIN_TOP,  domObject.getMargin().get(Spacing.TOP),
                        WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(value, domObject.getViewPortWidth()), domObject.getViewPortWidth()));
            }
            break;
            case Constants.Name.MARGIN_LEFT:{
                holder = PropertyValuesHolder.ofFloat(Constants.Name.MARGIN_LEFT,  domObject.getMargin().get(Spacing.LEFT),
                        WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(value, domObject.getViewPortWidth()), domObject.getViewPortWidth()));
            }
            break;
            case Constants.Name.MARGIN_RIGHT:{
                holder = PropertyValuesHolder.ofFloat(Constants.Name.MARGIN_RIGHT,  domObject.getMargin().get(Spacing.RIGHT),
                        WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(value, domObject.getViewPortWidth()), domObject.getViewPortWidth()));
            }
            break;
            case Constants.Name.MARGIN_BOTTOM:{
                holder = PropertyValuesHolder.ofFloat(Constants.Name.MARGIN_BOTTOM,  domObject.getMargin().get(Spacing.BOTTOM),
                        WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(value, domObject.getViewPortWidth()), domObject.getViewPortWidth()));
            }
            break;
            case Constants.Name.OPACITY:{
                DOMActionContext domActionContext = WXSDKManager.getInstance().getWXDomManager().getDomContext(domObject.getDomContext().getInstanceId());
                if(domActionContext != null){
                    WXComponent component = domActionContext.getCompByRef(domObject.getRef());
                    if(component != null && component.getHostView() != null){
                        holder = PropertyValuesHolder.ofFloat(Constants.Name.OPACITY,  component.getHostView().getAlpha(), WXUtils.getFloat(value, 0.0f));
                    }
                }
            }
            break;
            case Constants.Name.BACKGROUND_COLOR:{
                DOMActionContext domActionContext = WXSDKManager.getInstance().getWXDomManager().getDomContext(domObject.getDomContext().getInstanceId());
                if(domActionContext != null){
                    WXComponent component = domActionContext.getCompByRef(domObject.getRef());
                    if(component != null && component.getHostView() != null){
                        int color = WXResourceUtils.getColor(WXUtils.getString(domObject.getStyles().getBackgroundColor(), null), 0);
                        holder = PropertyValuesHolder.ofInt(Constants.Name.BACKGROUND_COLOR, color, WXResourceUtils.getColor(WXUtils.getString(value, null), 0));
                        holder.setEvaluator(new ArgbEvaluator());
                    }
                }
            }
            break;
            default:
                break;
        }
        if(holder == null){
            holder  = PropertyValuesHolder.ofFloat(property, 1, 1);
        }
        return  holder;
    }



    private void doPropertyValuesHolderAnimation(PropertyValuesHolder[] holders){
        valueAnimator = ValueAnimator.ofPropertyValuesHolder(holders);
        valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                PropertyValuesHolder holders[] = animation.getValues();
                for(PropertyValuesHolder holder : holders){
                    String property =  holder.getPropertyName();
                    if(WXEnvironment.isApkDebugable()){
                        WXLogUtils.d("WXTransition on property  " + property + animation.getAnimatedValue(property)
                        + " node ref " + domObject.getRef());
                    }
                    Log.e("weex", "work used " + property  + "   " + animation.getAnimatedValue(property));
                    switch (property){
                        case Constants.Name.WIDTH:{
                            domObject.setStyleWidth((Float) animation.getAnimatedValue(property));
                        }
                        break;
                        case Constants.Name.HEIGHT:{
                            domObject.setStyleHeight((Float) animation.getAnimatedValue(property));
                        }
                        break;
                        case Constants.Name.MARGIN_TOP:{
                            domObject.setMargin(Spacing.TOP, (Float) animation.getAnimatedValue(property));
                        }
                        break;
                        case Constants.Name.MARGIN_LEFT:{
                            domObject.setMargin(Spacing.LEFT, (Float) animation.getAnimatedValue(property));
                        }
                        break;
                        case Constants.Name.MARGIN_RIGHT:{
                            domObject.setMargin(Spacing.RIGHT, (Float) animation.getAnimatedValue(property));
                        }
                        break;
                        case Constants.Name.MARGIN_BOTTOM:{
                            domObject.setMargin(Spacing.BOTTOM, (Float) animation.getAnimatedValue(property));
                        }
                        break;
                        case Constants.Name.OPACITY:{
                            DOMActionContext domActionContext = WXSDKManager.getInstance().getWXDomManager().getDomContext(domObject.getDomContext().getInstanceId());
                            if(domActionContext != null){
                                WXComponent component = domActionContext.getCompByRef(domObject.getRef());
                                if(component != null && component.getHostView() != null){
                                    component.setAlpha((Float) animation.getAnimatedValue(property));
                                }
                            }
                        }
                        break;
                        case Constants.Name.BACKGROUND_COLOR:{
                            DOMActionContext domActionContext = WXSDKManager.getInstance().getWXDomManager().getDomContext(domObject.getDomContext().getInstanceId());
                            if(domActionContext != null){
                                WXComponent component = domActionContext.getCompByRef(domObject.getRef());
                                if(component != null && component.getHostView() != null){
                                    component.setBackgroundColor((Integer) animation.getAnimatedValue(property));
                                }
                            }
                        }
                        break;
                        default:
                            break;
                    }
                }

                DOMActionContext domActionContext = WXSDKManager.getInstance().getWXDomManager().getDomContext(domObject.getDomContext().getInstanceId());
                if(domActionContext == null){
                    return;
                }
                domActionContext.markDirty();
                WXSDKManager.getInstance().getWXDomManager().sendEmptyMessageDelayed(WXDomHandler.MsgType.WX_DOM_TRANSITION_BATCH, 0);

                if(WXEnvironment.isApkDebugable()){
                    WXLogUtils.d("WXTransition on property notify batch " +  domObject.getRef());
                }
            }
        });
        valueAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animationEnd) {
                ValueAnimator  animation = (ValueAnimator) animationEnd;
                super.onAnimationEnd(animation);
                if(WXEnvironment.isApkDebugable()){
                    WXLogUtils.d("WXTransition onAnimationEnd " +  domObject.getRef());
                }
                PropertyValuesHolder holders[] = animation.getValues();
                for(PropertyValuesHolder holder : holders){
                    String property =  holder.getPropertyName();
                    if(targetStyles.containsKey(property)){
                        domObject.getStyles().put(property, targetStyles.remove(property));
                    }
                }
            }
        });
        if(interpolator != null) {
            valueAnimator.setInterpolator(interpolator);
        }
        valueAnimator.setStartDelay((long) (delay));
        valueAnimator.setDuration((long) (duration));
        valueAnimator.start();
    }

    /**
     * create transition from map, update map properties.
     * */
    public static WXTransition fromMap(Map<String, Object> style, WXDomObject domObject){
        if(style.get(TRANSITION_PROPERTY) == null){
            return  null;
        }
        String propertyString = WXUtils.getString(style.get(TRANSITION_PROPERTY), null);
        if(propertyString == null){
            return null;
        }
        WXTransition transition  = new WXTransition();
        String[] propertiesArray = PROPERTY_SPLIT_PATTERN.split(propertyString);
        for(String property : propertiesArray){
            String trim = property.trim();
            if(TextUtils.isEmpty(trim)){
                continue;
            }
            if(!SUPPORTED_PROPERTIES.contains(trim)){
                if(WXEnvironment.isApkDebugable()){
                    WXLogUtils.e("WXTransition Property Not Supported" + trim + " in " + propertyString);
                }
                continue;
            }
            transition.properties.add(trim);
        }
        if(transition.properties.isEmpty()){
            return  null;
        }
        transition.duration = parseTimeMillis(style, TRANSITION_DURATION, 1);
        transition.delay =  parseTimeMillis(style, TRANSITION_DELAY, 0);
        transition.interpolator = createTimeInterpolator(WXUtils.getString(style.get(TRANSITION_TIMING_FUNCTION), null));
        transition.domObject = domObject;
        return  transition;
    }

    
    private static float parseTimeMillis(Map<String, Object> style, String key, float defaultValue){
        String  duration = WXUtils.getString(style.get(key), null);
        if(duration != null){
            duration = duration.replaceAll("s", "");
        }
        if(TextUtils.isEmpty(duration)){
            return  defaultValue;
        }
        try{
           return Float.parseFloat(duration);
        }catch (NumberFormatException e){
            return  defaultValue;
        }
    }


    private static Interpolator createTimeInterpolator(String interpolator) {
        if (!TextUtils.isEmpty(interpolator)) {
            switch (interpolator) {
                case EASE_IN:
                    return PathInterpolatorCompat.create(0.42f,0f, 1f,1f);
                case EASE_OUT:
                    return PathInterpolatorCompat.create(0f,0f, 0.58f,1f);
                case EASE_IN_OUT:
                    return PathInterpolatorCompat.create(0.42f,0f, 0.58f,1f);
                case LINEAR:
                    return PathInterpolatorCompat.create(0.0f,0f, 1f,1f);
                default:
                    //Parse cubic-bezier
                    try {
                        SingleFunctionParser<Float> parser = new SingleFunctionParser<>(
                                interpolator,
                                new SingleFunctionParser.FlatMapper<Float>() {
                                    @Override
                                    public Float map(String raw) {
                                        return Float.parseFloat(raw);
                                    }
                                });
                        List<Float> params = parser.parse(CUBIC_BEZIER);
                        if (params != null && params.size() == 4) {
                            return PathInterpolatorCompat.create(
                                    params.get(0), params.get(1), params.get(2), params.get(3));
                        }
                    } catch (RuntimeException e) {
                        WXLogUtils.e("WXTransition", e);
                    }
            }
        }
        return PathInterpolatorCompat.create(0.25f,0.1f, 0.25f,1f);
    }

}
