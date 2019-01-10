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

#include "hurricane/Hurricane.h"

#include "hurricane/base/NetListener.h"
#include "eventqueue.h"
#include "eventqueueloop.h"
#include "IoLoop.h"
#include "utils/logger.h"

#include <iostream>
#include <thread>
#include <chrono>

// 这个常量是网络监听器中使用的数据缓冲区的大小
// 为什么设置为65535? 网络中的数据都是使用IP报文发送的,而IP报文中表示数据长度的字段为2字节,也就是16位,因此能表示的最大长度为65535
const int DATA_BUFFER_SIZE = 65535;

// 该函数的第一步是创建一个新的TCPserver对象,并将其保存到一个std::shared_ptr智能指针中
void NetListener::StartListen()
{
    meshy::IoLoop::Get()->Start();

    _server.Listen(_host.GetHost(), _host.GetPort());
    _server.OnConnectIndication([this](meshy::IStream* stream) {
        stream->OnDataIndication([stream, this](const char* buf, int64_t size) mutable {
            this->_receiver(dynamic_cast<meshy::TcpStream*>(stream), buf, size);
        });
    });
}