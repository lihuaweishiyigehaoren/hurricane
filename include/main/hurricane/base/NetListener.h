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

#include "hurricane/base/NetAddress.h"
#include "net.h"
#include "Meshy.h"

#include <vector>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <functional>
#include <cstdint>

// 数据接收处理函数
typedef std::function<void(meshy::TcpStream* connection,
    const char* buffer, int32_t size)> 
        DataReceiver;

// 网络监听器
// 基于tcp/IP套接字的网络消息监听类,可以帮助我们进行网络监听
class NetListener {
public:
    // 构造函数参数-需要监听的网络地址,(主机名+地址)
    NetListener(const hurricane::base::NetAddress& host) :
            _host(host) {
    }

    const hurricane::base::NetAddress& GetHost() const {
        return _host;
    }

    void SetHost(const hurricane::base::NetAddress& host) {
        _host = host;
    }

    void StartListen();

    // 帮助用户注册监听数据消息的回调函数,当网络监听器接收到消息时,会调用用户注册的回调函数
    void OnData(DataReceiver receiver) {
        _receiver = receiver;
    }

private:
    hurricane::base::NetAddress _host;
    DataReceiver _receiver;
    meshy::TcpServer _server;
};