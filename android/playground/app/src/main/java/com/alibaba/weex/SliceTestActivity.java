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
package com.alibaba.weex;

import android.content.res.AssetFileDescriptor;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;
import com.taobao.weex.IWXRenderListener;
import com.taobao.weex.WXSDKEngine;
import com.taobao.weex.WXSDKInstance;
import com.taobao.weex.annotation.JSMethod;
import com.taobao.weex.common.WXException;
import com.taobao.weex.common.WXModule;
import com.taobao.weex.common.WXRenderStrategy;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import static com.taobao.weex.WXSDKInstance.BUNDLE_URL;

public class SliceTestActivity extends AppCompatActivity {
  private static final String LOG_TAG = "SliceTestActivity";
  private static final boolean USING_BIN = true;
  private RecyclerView mRecyclerView;
  private TextView mReportTextView;

  private final List<String> mData = new ArrayList<>();
  private WXInstanceAdapter mAdapter;
  private final Set<WXSDKInstance> mInstances = new HashSet<>();
  private String mDataTmp;

  public static class SearchModule extends WXModule {
    @JSMethod(uiThread = true)
    public void search(JSONObject options) {
      Log.e("TestModuel", options.toJSONString());
    }
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    try {
      WXSDKEngine.registerModule("searchEvent", SearchModule.class);
    } catch (WXException e) {
      e.printStackTrace();
    }
    setContentView(R.layout.activity_slice_test);
    mRecyclerView = (RecyclerView) findViewById(R.id.recycler_view);
    mReportTextView = (TextView) findViewById(R.id.report_text);

    mRecyclerView.setLayoutManager(new LinearLayoutManager(this));
    mRecyclerView.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));
    mAdapter = new WXInstanceAdapter();
    mRecyclerView.setAdapter(mAdapter);
    mDataTmp = loadAssets("lite_template/test.json");
  }

  static int i = 0;

  public void addCellClick(View view) {
    //rax case.js
    JSONObject obj = JSON.parseObject(mDataTmp);
    if(i++ % 2 == 0){
      obj.getJSONObject("status").put("layoutStyle",0);
    } else {
      obj.getJSONObject("status").put("layoutStyle",1);
    }
    mData.add(obj.toJSONString());
    mAdapter.notifyItemInserted(mData.size() - 1);
  }

  private class WXInstanceAdapter extends RecyclerView.Adapter<WXViewHolder> {
    @Override
    public WXViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
      Log.d(LOG_TAG, "onCreateViewHolder");
      FrameLayout itemView = new FrameLayout(SliceTestActivity.this);
      itemView.setLayoutParams(new RecyclerView.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
      return new WXViewHolder(itemView);
    }

    @Override
    public void onBindViewHolder(WXViewHolder holder, int position) {
      String data = mData.get(position);
      if (!holder.isRendered()) {
        Log.d(LOG_TAG, "render onBindViewHolder " + position);
        holder.render(data, position);
      } else {
        Log.d(LOG_TAG, "refresh onBindViewHolder " + position);
        holder.refresh(data, position);
      }
    }

    @Override
    public int getItemCount() {
      return mData.size();
    }
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    for (WXSDKInstance instance : mInstances) {
      instance.destroy();
    }
    mInstances.clear();
  }

  private class WXViewHolder extends RecyclerView.ViewHolder implements IWXRenderListener {
    private WXSDKInstance mInstance;
    private boolean mRendered;

    private TextView mTextView;

    public WXViewHolder(View itemView) {
      super(itemView);
      mInstance = new WXSDKInstance(SliceTestActivity.this);
      mInstance.registerRenderListener(this);
      mInstances.add(mInstance);
      mTextView = new TextView(SliceTestActivity.this);
      FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
      params.gravity = Gravity.RIGHT;
      ((ViewGroup) itemView).addView(mTextView, params);
    }

    public void render(String initData, int position) {
      Map<String,Object> options = new HashMap<>();
      options.put(BUNDLE_URL, "slice/test.js");
      if (!USING_BIN) {
        mInstance.render(
            "testPage",
            loadAssets("lite_template/test.js"),
            options,
            initData,
            WXRenderStrategy.DATA_RENDER
        );
      } else {
        mInstance.render(
            "testPage",
            loadBytes(),
            options,
            initData
        );
      }
      mTextView.setText(String.valueOf(position));
      mRendered = true;
    }

    public boolean isRendered() {
      return mRendered;
    }

    public void refresh(String initData, int position) {
      mInstance.refreshInstance(initData);
      mTextView.setText(String.valueOf(position));
    }

    @Override
    public void onViewCreated(WXSDKInstance instance, View view) {
      ((ViewGroup) itemView).addView(view, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    @Override
    public void onRenderSuccess(WXSDKInstance instance, int width, int height) {
    }

    @Override
    public void onRefreshSuccess(WXSDKInstance instance, int width, int height) {

    }

    @Override
    public void onException(WXSDKInstance instance, String errCode, String msg) {

    }
  }

  @NonNull
  private String loadAssets(String fileName) {
    StringBuilder buf = new StringBuilder();
    try {
      InputStream json = getAssets().open(fileName);
      BufferedReader in =
          new BufferedReader(new InputStreamReader(json, "UTF-8"));
      String str;

      while ((str = in.readLine()) != null) {
        buf.append(str);
      }

      in.close();
    } catch (IOException e) {
      e.printStackTrace();
    }
    return buf.toString();
  }



  private byte[] loadBytes() {
    try {
      AssetFileDescriptor assetFileDescriptor = getAssets().openFd("lite_template/test.wasm");
      long len = assetFileDescriptor.getDeclaredLength();
      ByteBuffer buf = ByteBuffer.allocate((int) len);
      InputStream json = assetFileDescriptor.createInputStream();
      json.read(buf.array());
      json.close();
      return buf.array();
    } catch (IOException e) {
      e.printStackTrace();
    }
    return null;
  }
}
