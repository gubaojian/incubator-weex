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
package com.taobao.weex.render.frame;

import android.os.Handler;

import com.taobao.weex.render.log.RenderLog;

import java.lang.ref.WeakReference;

public class FrameController {

    private static final long DEFAULT_FRAME = 1000 / 60;

    private WeakReference<FrameListener> mFrameListener;
    private Handler mFrameHandler;
    private long mFrameTime;
    private Runnable mFrameRunnable;
    private volatile boolean mRequestFrame;

    public FrameController(Handler frameHandler, FrameListener frameListener) {
        this.mFrameHandler = frameHandler;
        this.mFrameListener = new WeakReference<>(frameListener);
        this.mFrameTime = DEFAULT_FRAME;
        this.mFrameRunnable = new Runnable() {
            @Override
            public void run() {
                FrameListener frameListener;
                if(mFrameListener != null && (frameListener = mFrameListener.get()) != null){
                    try {
                        if(mRequestFrame){
                            mRequestFrame = false;
                        }
                        frameListener.onFrame();
                    }catch (Throwable e){
                        RenderLog.onError("FrameController runnable ", e);
                    }
                }
            }
        };
    }



    public void requestFrame() {
        if(!mRequestFrame){
            mRequestFrame = true;
            mFrameHandler.postDelayed(mFrameRunnable, mFrameTime);
        }
    }


    public void requestImmediately(){
        mRequestFrame = true;
        mFrameHandler.postAtFrontOfQueue(mFrameRunnable);
    }



    public interface FrameListener {
        void onFrame();
    }
}
