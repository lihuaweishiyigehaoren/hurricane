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

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace hurricane {

namespace message {
    class Message;

    // 消息队列接口
    class MessageLoop {
    public:
        // 消息的处理函数的类型定义 
        // 这里使用C++11的std::function类型作为回调函数的类型,因为该类型可以存储函数指针,也可以存储lambda表达式
        typedef std::function<void(Message*)> MessageHandler;

        MessageLoop();
        MessageLoop(const MessageLoop&) = delete;// 不可复制
        const MessageLoop& operator=(const MessageLoop&) = delete;

        // 实现某个消息类型和消息处理函数之间的映射关系
        /*
        #执行过程
        1、将消息类型和消息处理函数的std:;function包装存放在一个映射表中
        2、根据消息类型索引得到消息处理函数
        3、执行获取到的function对象
        */
        template <class ObjectType, class MethodType>
        void MessageMap(int messageType, ObjectType* self, MethodType method) {
            MessageMap(messageType, std::bind(method, self, std::placeholders::_1));
        }

        void MessageMap(int messageType, MessageHandler handler) {
            _messageHandlers.insert({ messageType, handler });
        }

        // 负责启动消息队列
        void Run();
        // 停止消息队列
        void Stop();
        // 负责向消息队列里投递消息
        void PostMessage(Message* message);

    private:
        std::map<int, MessageHandler> _messageHandlers;
        uint64_t _threadId;
    };

    // 管理消息循环
    // 该类的作用是解耦合,将所有的消息队列存放到一个映射表中,这样每一个名字就可以对应某一个消息队列
    class MessageLoopManager {
    public:
        // GETINstance 将该类型单例化,这种使用函数局部的静态变量是C++中的常用技巧,用于解决初始化依赖问题
        static MessageLoopManager& GetInstance() {
            static MessageLoopManager manager;

            return manager;
        }

        MessageLoopManager(const MessageLoopManager&) = delete;
        const MessageLoopManager& operator=(const MessageLoopManager&) = delete;

        // 注册消息队列,注册的时候只需要提供对列名和队列实例
        void Register(const std::string& name, MessageLoop* loop) {
            _messageLoops.insert({ name, std::shared_ptr<MessageLoop>(loop) });
        }
        
        // 传递消息,只需要使用消息队列的名称就可以将消息投递到对应的消息队列了
        void PostMessage(const std::string& name, Message* message) {
            auto messageLoopPair = _messageLoops.find(name);
            if ( messageLoopPair != _messageLoops.end() ) {
                messageLoopPair->second->PostMessage(message);
            }
        }

    private:
        MessageLoopManager() {}

        std::map<std::string, std::shared_ptr<MessageLoop>> _messageLoops;
    };
}

}