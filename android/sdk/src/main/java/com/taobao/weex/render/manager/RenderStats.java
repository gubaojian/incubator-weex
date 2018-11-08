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
package com.taobao.weex.render.manager;

import android.util.Log;

import com.taobao.weex.render.log.RenderLog;

import java.util.concurrent.atomic.AtomicInteger;

public class RenderStats {


    public static int MAX_DETTACH_NUM_ON_SECOND = 10;

    private static AtomicInteger elgNum = new AtomicInteger(0);
    private static volatile AtomicInteger currentWaitEglTaskNum = new AtomicInteger(0);
    private static final int MAX_WAIT_EGL_TASK_NUM = 8;

    public static AtomicInteger getElgNum(){
        return elgNum;
    }

    public static AtomicInteger getCurrentWaitEglTaskNum(){
        return currentWaitEglTaskNum;
    }

    public static void waitIfWaitEGLTaskExceed(){
        showRenderStats();
        int maxTimes = currentWaitEglTaskNum.get();
        while (currentWaitEglTaskNum.get() >= MAX_WAIT_EGL_TASK_NUM && maxTimes > 0){
            try {
                Thread.sleep(4);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            maxTimes--;
        }
    }

    public static void showRenderStats() {
        if(RenderLog.isDebugLogEnabled()){
            Log.e(RenderLog.RENDER_LOG_TAG,
                    "EGLControl stats frameNum "
                            + RenderManager.getInstance().getRenderFrameMap().size()
                            + " waitNum " + currentWaitEglTaskNum.get()
                            + " eglNum " + elgNum.get());
        }
    }

    public static int getCountDettachNum() {
        return dettachNum;

    }
    public static void countDettachNum() {
        if((System.currentTimeMillis() - lastCountTime) > 1500){
            lastCountTime = System.currentTimeMillis();
            dettachNum = 1;
        }else{
            dettachNum++;
            if(dettachNum > MAX_DETTACH_NUM_ON_SECOND){
                try {
                    Thread.sleep(sleepTime*(dettachNum - MAX_DETTACH_NUM_ON_SECOND));
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
        Log.e("Weex", "Weex dettach num " + dettachNum);
    }

    private static long sleepTime = 4;
    private static long lastCountTime;
    private static int dettachNum;
}
