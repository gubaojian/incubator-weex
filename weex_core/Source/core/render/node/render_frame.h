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
//
// Created by furture on 2018/8/7.
//
#ifndef CORE_RENDER_NODE_RENDER_FRAME_H
#define CORE_RENDER_NODE_RENDER_FRAME_H

#include <map>
#include <string>
#include <vector>

#include "core/css/constants_value.h"
#include "core/render/node/render_object.h"
#include "core/render/node/factory/render_type.h"

/**
 * render document node for  all the tag in the block will not in layout tree
 * */
namespace WeexCore {

    bool isRenderFrameChild(RenderObject *node);

    class RenderFrame : public RenderObject  {

    public:
        ~RenderFrame();

        virtual void removeChild(const WXCoreLayoutNode* const child) override;
        virtual void addChildAt(WXCoreLayoutNode* const child, Index index) override;
        virtual int AddRenderObject(int index, RenderObject *child) override;
        RenderObject *GetChild(const Index &index);
        void RemoveRenderObject(RenderObject *child);

        StyleType ApplyStyle(const std::string &key, const std::string &value,
                             const bool updating);

        std::vector<RenderObject *> &GetFrameChilds(){
            return frameChilds;
        }

    public:
        static void setFrameSwitchOpen(bool switchOpen){
            frameSwitchOpen = switchOpen;
        }

        static bool isFrameSwitchOpen(){
            return frameSwitchOpen;
        }

    private:
        std::vector<RenderObject *> frameChilds;
        static bool frameSwitchOpen;
    };
}


#endif //WEEX_PROJECT_RENDER_BLOCK_H
