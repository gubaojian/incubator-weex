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
import android.animation.PropertyValuesHolder;
import android.animation.ValueAnimator;
import android.support.v4.util.ArrayMap;
import android.text.TextUtils;
import android.util.Log;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;
import android.view.animation.LinearInterpolator;

import com.taobao.weex.WXSDKManager;
import com.taobao.weex.common.Constants;
import com.taobao.weex.dom.DOMActionContext;
import com.taobao.weex.dom.WXDomHandler;
import com.taobao.weex.dom.WXDomObject;
import com.taobao.weex.dom.WXStyle;
import com.taobao.weex.dom.flex.Spacing;
import com.taobao.weex.utils.WXUtils;
import com.taobao.weex.utils.WXViewUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;

import static com.taobao.weex.common.Constants.TimeFunction.EASE_IN;
import static com.taobao.weex.common.Constants.TimeFunction.EASE_IN_OUT;
import static com.taobao.weex.common.Constants.TimeFunction.EASE_OUT;
import static com.taobao.weex.common.Constants.TimeFunction.LINEAR;

/**
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
    public static final Pattern PROPERTY_SPLIT_PATTERN = Pattern.compile("\\|");


    private List<String> properties;
    private Interpolator  interpolator;
    private float  duration;
    private float delay;
    private Map<String, Object> targetStyles;
    private ValueAnimator valueAnimator;
    private WXDomObject domObject;

    public WXTransition() {
        this.properties = new ArrayList<>(4);
        this.targetStyles = new ArrayMap<>();
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
                targetStyles.put(property, targetValue);
            }else{
                if(targetStyles.containsKey(property)){
                    domObject.getStyles().put(property, targetStyles.remove(property));
                }
            }
        }
        if(targetStyles.size() == 0){
            return;
        }

        PropertyValuesHolder[] holders = new  PropertyValuesHolder[targetStyles.size()];
        Set<Map.Entry<String, Object>> styleEntries = targetStyles.entrySet();
        WXStyle style = domObject.getStyles();
        int index = 0 ;
        for(Map.Entry<String, Object> entry : styleEntries){
              String property =  entry.getKey();
              switch (property){
                   case Constants.Name.WIDTH:{
                       holders[index] = PropertyValuesHolder.ofFloat(Constants.Name.WIDTH, domObject.getLayoutWidth(),
                               WXViewUtils.getRealPxByWidth(WXUtils.getFloat(entry.getValue()), domObject.getViewPortWidth()));
                   }
                   break;
                   case Constants.Name.HEIGHT:{
                      holders[index] = PropertyValuesHolder.ofFloat(Constants.Name.HEIGHT, domObject.getLayoutHeight(),
                              WXViewUtils.getRealPxByWidth(WXUtils.getFloat(entry.getValue()), domObject.getViewPortWidth()));
                   }
                   break;
                  case Constants.Name.MARGIN_TOP:{
                      holders[index] = PropertyValuesHolder.ofFloat(Constants.Name.MARGIN_TOP,  domObject.getMargin().get(Spacing.TOP),
                              WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(entry.getValue(), domObject.getViewPortWidth()), domObject.getViewPortWidth()));
                  }
                  break;
                  case Constants.Name.MARGIN_LEFT:{
                      holders[index] = PropertyValuesHolder.ofFloat(Constants.Name.MARGIN_LEFT,  domObject.getMargin().get(Spacing.LEFT),
                              WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(entry.getValue(), domObject.getViewPortWidth()), domObject.getViewPortWidth()));
                  }
                  break;
                  case Constants.Name.MARGIN_RIGHT:{
                      holders[index] = PropertyValuesHolder.ofFloat(Constants.Name.MARGIN_RIGHT,  domObject.getMargin().get(Spacing.RIGHT),
                              WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(entry.getValue(), domObject.getViewPortWidth()), domObject.getViewPortWidth()));
                  }
                  break;
                  case Constants.Name.MARGIN_BOTTOM:{
                      holders[index] = PropertyValuesHolder.ofFloat(Constants.Name.MARGIN_BOTTOM,  domObject.getMargin().get(Spacing.BOTTOM),
                              WXViewUtils.getRealPxByWidth(WXUtils.getFloatByViewport(entry.getValue(), domObject.getViewPortWidth()), domObject.getViewPortWidth()));
                  }
                  break;
                  default:
                      holders[index] = PropertyValuesHolder.ofFloat(property, 1, 1);
                      break;
              }
        }
        if(valueAnimator != null){
            valueAnimator.cancel();
        }
        valueAnimator = ValueAnimator.ofPropertyValuesHolder(holders);
        valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                PropertyValuesHolder holders[] = animation.getValues();
                for(PropertyValuesHolder holder : holders){
                    String property =  holder.getPropertyName();
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
                        default:
                            break;
                    }
                }

                DOMActionContext domActionContext = WXSDKManager.getInstance().getWXDomManager().getDomContext(domObject.getDomContext().getInstanceId());
                if(domActionContext == null){
                    return;
                }
                long start = System.currentTimeMillis();
                domActionContext.forceBatch();
                Log.e("weex", "work used "  + (System.currentTimeMillis() - start));
            }
        });
        valueAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                for(String property : properties){
                    if(targetStyles.containsKey(property)){
                        domObject.getStyles().put(property, targetStyles.remove(property));
                    }
                }
            }
        });
        valueAnimator.setInterpolator(interpolator);
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
            transition.properties.add(trim);
        }
        transition.duration = getSecond(style, TRANSITION_DURATION, 1);
        transition.delay =  getSecond(style, TRANSITION_DELAY, 0);
        transition.interpolator = createTimeInterpolator(WXUtils.getString(style.get(TRANSITION_TIMING_FUNCTION), null));
        transition.domObject = domObject;
        return  transition;
    }





    private static float getSecond(Map<String, Object> style, String key, float defaultValue){
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
                    return new AccelerateInterpolator();
                case EASE_OUT:
                    return new DecelerateInterpolator();
                case EASE_IN_OUT:
                    return new AccelerateDecelerateInterpolator();
                case LINEAR:
                    return new LinearInterpolator();
                default:
                    /**
                    //Parse cubic-bezier
                    try {
                        SingleFunctionParser<Float> parser = new SingleFunctionParser<>(
                                mAnimationBean.timingFunction,
                                new SingleFunctionParser.FlatMapper<Float>() {
                                    @Override
                                    public Float map(String raw) {
                                        return Float.parseFloat(raw);
                                    }
                                });
                        List<Float> params = parser.parse(CUBIC_BEZIER);
                        if (params != null && params.size() == WXAnimationBean.NUM_CUBIC_PARAM) {
                            return PathInterpolatorCompat.create(
                                    params.get(0), params.get(1), params.get(2), params.get(3));
                        } else {
                            return null;
                        }
                    } catch (RuntimeException e) {
                        return null;
                    }*/
            }
        }
        return new LinearInterpolator();
    }

}
