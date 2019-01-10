/**
 * licensed to the apache software foundation (asf) under one
 * or more contributor license agreements.  see the notice file
 * distributed with this work for additional information
 * regarding copyright ownership.  the asf licenses this file
 * to you under the apache license, version 2.0 (the
 * "license"); you may not use this file except in compliance
 * with the license.  you may obtain a copy of the license at
 *
 * http://www.apache.org/licenses/license-2.0
 *
 * unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "as is" basis,
 * without warranties or conditions of any kind, either express or implied.
 * see the license for the specific language governing permissions and
 * limitations under the license.
 */

#pragma once

#include "hurricane/base/ITask.h"

namespace hurricane {

    namespace base {

        class OutputCollector;

    }

        namespace spout {

        class ISpout : public base::ITask {
        public:
			// Open 负责打开并初始化一个新的消息源
            // OutputCollector 是hurricane的数据收集器,负责收集数据并将数据传给其他的Bolt节点
            virtual void Open(base::OutputCollector& outputCollector) = 0;

			// Close 用来关闭消息源,在拓扑结构停止时会被调用,来清理消息源对象使用的环境资源
            virtual void Close() = 0;

			// Execute 任务执行器会不断执行该任务,并通过该任务不断地向拓扑结构中输入数据,所以消息源是拓扑结构中所有消息流的起点
            virtual void Execute() = 0;
			
			// 在堆上产生对象自身的一份副本,并将复制对象的指针返回
            // 任务执行器会使用该方法来根据用户定义的Spout复制生成任务
            virtual ISpout* Clone() const = 0;
        };

    }
}

