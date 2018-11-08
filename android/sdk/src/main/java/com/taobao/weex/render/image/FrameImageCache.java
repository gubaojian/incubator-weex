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
package com.taobao.weex.render.image;

import android.util.LruCache;

import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Created by furture on 2018/8/20.
 */

public class FrameImageCache {

    private LruCache<Long,WeakReference<Map<String, FrameImage>>> frameImageCaches;

    private FrameImageCache(){
         frameImageCaches = new LruCache<>(512);
    }


    private static FrameImageCache cacheInstance;

    public static FrameImageCache getInstance(){
        if(cacheInstance == null){
            synchronized (FrameImageCache.class){
                if(cacheInstance == null) {
                    cacheInstance = new FrameImageCache();
                }
            }
        }
        return cacheInstance;
    }


    public FrameImage getFrameImage(long documentKey, String imageUrl){
        Map<String, FrameImage> cache = getFrameImageMap(documentKey);
        if(cache == null){
            return null;
        }
        return cache.get(imageUrl);
    }

    private Map<String, FrameImage> getFrameImageMap(long documentKey){
        WeakReference<Map<String, FrameImage>> imageTargetWeakReference = frameImageCaches.get(documentKey);
        if(imageTargetWeakReference == null){
            return null;
        }
        Map<String, FrameImage> imageTargetMap = imageTargetWeakReference.get();
        return imageTargetMap;
    }


    public void cacheFrameImages(long documentKey, Map<String, FrameImage> frameImageMap){
        if(frameImageMap == null || frameImageMap.size() == 0) {
            return;
        }

        Map<String, FrameImage> caches =  getFrameImageMap(documentKey);
        if(caches == null){
            caches = new ConcurrentHashMap<>();
            frameImageCaches.put(documentKey, new WeakReference<Map<String, FrameImage>>(caches));
        }
        caches.clear();
        Set<Map.Entry<String, FrameImage>> entrySet =  frameImageMap.entrySet();
        for(Map.Entry<String, FrameImage> entry : entrySet){
            caches.put(entry.getKey(), entry.getValue());
        }
    }


    public void clear(){
        frameImageCaches.evictAll();
    }


}
