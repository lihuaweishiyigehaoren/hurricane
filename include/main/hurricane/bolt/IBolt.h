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
        class Values;

    }

    namespace bolt {

        class IBolt : public base::ITask {
        public:
			// 在任务启动之前对任务对象进行初始化,和消息源中的Open极为相似,在整个任务的生命周期内只会被调用一次
            virtual void Prepare(base::OutputCollector& outputCollector) = 0;
			// 作用:在拓扑结构停止时对任务的资源进行清理,在整个任务的声明周期内也只会被调用一次
            virtual void Cleanup() = 0;
			// 该函数和消息源中Execute方法一样,会被不断的执行
            // 但又不同,消息源中的Execute会被主动反复执行,而消息处理器中这个Execute则属于被动执行-只有在其他的处理节点的数据到来时才会调用该成员函数,
            // 因此没有数据到来的时候,该函数处于阻塞状态
            virtual void Execute(const base::Values& values) = 0;

			// 该函数的作用和数据源中的CLone一样,用于复制任务对象
            virtual IBolt* Clone() const = 0;
        };

    }
}

