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
package com.alibaba.weex.extend.render;

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import com.squareup.picasso.Picasso;
import com.squareup.picasso.Target;
import com.taobao.weex.render.frame.RenderFrame;
import com.taobao.weex.render.image.FrameImage;
import com.taobao.weex.render.image.FrameImageProcessor;
import com.taobao.weex.render.frame.RenderFrame;

/**
 * Created by furture on 2018/7/24.
 */

public class PicassoImageTarget implements FrameImage, Target {
    private Bitmap mBitmap;
    private String ref;
    private String url;
    private boolean isPlaceholder;
    private int loadingState;
    private RenderFrame renderFrame;

    public PicassoImageTarget(RenderFrame renderFrame, String ref, String url, boolean isPlaceholder) {
        this.loadingState = LOADING;
        this.renderFrame = renderFrame;
        this.ref = ref;
        this.url = url;
        this.isPlaceholder = isPlaceholder;
    }

    @Override
    public String getRef() {
        return ref;
    }

    @Override
    public String getUrl() {
        return url;
    }

    @Override
    public boolean isPlaceholder() {
        return isPlaceholder;
    }

    @Override
    public Bitmap getBitmap() {
        return mBitmap;
    }

    @Override
    public int getLoadingState() {
        return loadingState;
    }


    @Override
    public void onBitmapLoaded(Bitmap bitmap, Picasso.LoadedFrom from) {
        mBitmap = bitmap;
        FrameImageProcessor.toRenderSupportImage(this);
    }

    @Override
    public void onSupportedBitmap(Bitmap bitmap) {
        mBitmap = bitmap;
        loadingState = LOADING_SUCCESS;
        renderFrame.onLoadFrameImage(this);
    }

    @Override
    public void onBitmapFailed(Drawable errorDrawable) {
        loadingState = LOADING_FAILED;
        renderFrame.onLoadFrameImage(this);
    }


    @Override
    public void onPrepareLoad(Drawable placeHolderDrawable) {

    }
}
