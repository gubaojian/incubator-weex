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

import android.util.Log;

import com.taobao.weex.render.manager.RenderManager;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by furture on 2018/8/1.
 */

public class GLTaskQueue implements Runnable {

    private List<GLTask> queues;

    public GLTaskQueue() {
        this.queues = new ArrayList<>();
    }


    @Override
    public void run() {
        List<GLTask> tasks = new ArrayList<>();
        synchronized (this){
            tasks.addAll(queues);
            queues.clear();
        }
        for(GLTask task : tasks){
            Log.e("Weex", "execute task " + task
                    + "  " +  tasks.size()
                    + "  " + queues.size());
            task.execute();
        }
        tasks.clear();

    }


    public void addTask(GLTask task){
        synchronized (this){
            queues.add(task);
        }
        RenderManager.getInstance().getGpuHandler().removeCallbacks(this);
        RenderManager.getInstance().getGpuHandler().postAtFrontOfQueue(this);
    }

    public boolean isEmpty(){
        return queues.isEmpty();
    }


    public int size() {
        return queues.size();
    }
}
