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

#include "hurricane/base/Values.h"
#include "hurricane/message/SupervisorCommander.h"

namespace hurricane {

    namespace topology {
    class ITopology;
    }

    namespace base {

		// 数据收集器-作用比较单一,负责进行数据传递(基础类型之一)
        class OutputCollector {
        public:
			// Strategy 指的是数据收集器的发送策略
            struct Strategy {
                enum Values {
					// 全局发送策略,在OutputCollector初始化的时候确定向一个固定的数据处理单元发送数据
                    // 在整个拓扑结构的运行过程中都不会改变,是一种最简单的数据发送策略
                    Global = 0,

					// 随机发送,在每次发送时都从合法的下一批目标节点中随机选择一个病发送到该数据处理单元.
                    // 由于每次都要和整个集群的中央节点通信以获得下一个消息处理单元的位置,所以效率相对于第一种策略低
                    Random = 1,

					// 分组发送策略,预先指定一个字段,在发送数据时,每次都会将字段相同的数据发送到某个固定的数据处理单元中.
                    // 这种方式第一次发送数据的时候会向集群的中央节点请求获取发送目标的位置,之后由于这个位置固定,因此不会再请求,故性能影响不大
                    Group = 2
                };
            };    

			/*
            # para1 消息收集器来源的任务名称,可能是消息源名称,也可能是消息处理器的名称
            # para2 消息发送策略,同上
            */
            OutputCollector(const std::string& src, int strategy) :
                _src(src), _strategy(strategy), _commander(nullptr) {}

			// 作用:发送一个元祖,具体实现中会根据数据收集器的发送策略发送元祖数据
            virtual void Emit(const Values& values);
			// 作用:设置命令执行器,命令执行器的作用:与网络上的其他节点进行通信
            // 这里我们已经将所有的原始数据抽象为高层的命令,而将通信层的负责细节隐藏在底层
            void SetCommander(hurricane::message::SupervisorCommander* commander) {
                if ( _commander ) {
                    delete _commander;
                }

                _commander = commander;
            }

			// _taskIndex是OutputCollector每次发送元祖数据时目的地的任务编号.
            // 一个Manager会管理多个任务,而Commander之关联到了某个Manager节点,但是将数据分发给哪个任务是不知道的,因此我们需要使用taskIndex做定位
            // Manager的位置和taskIndex的关系就像主机名和端口号,决定了发送的目标任务
            void SetTaskIndex(int taskIndex) {
                _taskIndex = taskIndex;
            }

			// groupField给分组策略使用,分组策略需要根据这个groupField将数据发送到某个固定的数据处理单元
            // groupField是字段在任务定义中的字段编号,这个字段编号结合字段列表就可以确定是哪一个字段
            void SetGroupField(int groupField) {
                _groupField = groupField;
            }

			// 获取分组字段编号
            int GetGroupField() const {
                return _groupField;
            }

			// 实现随机选择目的消息处理单元的功能
            int GetStrategy() const {
                return _strategy;
            }
			
			// 实现随机选择目的消息处理单元的功能
            virtual void RandomDestination() {};
			// 根据字段选择元组发送的目标消息处理单元
            virtual void GroupDestination() {};

        private:
            std::string _src;// 发送源的名称
            int _strategy;// 策略编号
            int _taskIndex;// 目标任务编号
            hurricane::message::SupervisorCommander* _commander;// 命令发送器,默认为空指针
            int _groupField;// 分组策略中,指定了分组使用的字段编号
        };

    }
}
