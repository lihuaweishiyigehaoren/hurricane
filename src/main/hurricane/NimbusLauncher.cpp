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

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "hurricane/base/NetAddress.h"
#include "hurricane/base/ByteArray.h"
#include "hurricane/base/DataPackage.h"
#include "hurricane/base/NetListener.h"
#include "hurricane/base/NetConnector.h"
#include "hurricane/message/CommandDispatcher.h"
#include "hurricane/message/NimbusCommander.h"
#include "hurricane/base/Node.h"
#include "hurricane/topology/ITopology.h"
#include "hurricane/spout/ISpout.h"
#include "hurricane/bolt/IBolt.h"
#include "Meshy.h"

using hurricane::base::NetAddress;
using hurricane::base::ByteArray;
using hurricane::base::DataPackage;
using hurricane::message::Command;
using hurricane::message::CommandDispatcher;
using hurricane::message::NimbusCommander;
using hurricane::base::Node;
using hurricane::topology::ITopology;
using hurricane::spout::ISpout;
using hurricane::bolt::IBolt;

hurricane::topology::ITopology* GetTopology();

const NetAddress NIMBUS_ADDRESS{ "127.0.0.1", 6000 };
const std::map<std::string, NetAddress> SUPERVISOR_ADDRESSES{
    { "s1", {"127.0.0.1", 7001} }
};

typedef std::vector<std::string> Tasks;
typedef std::map<std::string, int> ExecutorCounts;

const int EXECUTOR_CAPACITY = 3;
const int DEFAULT_SPOUT_EXECUTOR_COUNT = 1;
const int DEFAULT_BOLT_EXECUTOR_COUNT = 1;

static void dispatchTasks(std::map<std::string, Node>& supervisors,
        std::map<std::string, Tasks>& spoutTasks,
        std::map<std::string, Tasks>& boltTasks,
        ITopology* topology) {
    // TODO: Check the status of supervisors.
    // TODO: Rebalance the tasks of dead supervisors.

    ExecutorCounts neededExecutors;
    for ( const auto& pair : topology->GetBolts() ) {
        neededExecutors[pair.first] = DEFAULT_BOLT_EXECUTOR_COUNT;
    }

    for ( const auto& pair : topology->GetSpouts() ) {
        neededExecutors[pair.first] = DEFAULT_SPOUT_EXECUTOR_COUNT;
    }

    for ( auto& boltPair : topology->GetBolts() ) {
        std::string boltName = boltPair.first;
        std::shared_ptr<IBolt> bolt = boltPair.second;

        if ( neededExecutors[boltName] > 0 ) {
            for ( auto& pair : supervisors ) {
                std::string supervisorName = pair.first;
                Node& supervisorNode = pair.second;

                NimbusCommander commander(pair.second.GetAddress());

                std::cout << "Dispatch bolt on: " << supervisorName << std::endl;
                
                Tasks& tasks = boltTasks[supervisorName];
                for ( int taskIndex = 0; taskIndex != tasks.size(); ++ taskIndex ) {
                    if ( tasks[taskIndex].empty() ) {
                        tasks[taskIndex] = boltName;
                        neededExecutors[boltName] --;
                        commander.StartBolt(boltName, taskIndex);

                        if ( neededExecutors[boltName] == 0 ) {
                            break;
                        }
                    }
                }

                if ( neededExecutors[boltName] == 0 ) {
                    break;
                }
            }
        }
    }

    for ( auto& spoutPair : topology->GetSpouts() ) {
        std::string spoutName = spoutPair.first;
        std::shared_ptr<ISpout> spout = spoutPair.second;

        if ( neededExecutors[spoutName] > 0 ) {
            for ( auto& pair : supervisors ) {
                std::string supervisorName = pair.first;
                Node& supervisorNode = pair.second;

                NimbusCommander commander(pair.second.GetAddress());

                std::cout << "Dispatch spout on: " << supervisorName << std::endl;

                Tasks& tasks = spoutTasks[supervisorName];
                for ( int taskIndex = 0; taskIndex != tasks.size(); ++ taskIndex ) {
                    if ( tasks[taskIndex].empty() ) {
                        tasks[taskIndex] = spoutName;
                        neededExecutors[spoutName] --;
                        commander.StartSpout(spoutName, taskIndex);

                        if ( neededExecutors[spoutName] == 0 ) {
                            break;
                        }
                    }
                }

                if ( neededExecutors[spoutName] == 0 ) {
                    break;
                }
            }
        }
    }
}

std::pair<Node, int> GetFieldDestination(
    std::map <std::pair <std::string, std::string>,
    std::pair < Node, int>>& fieldDestinations,
    std::pair <std::string, std::string> fieldPair,
    ITopology* topology,
    std::map<std::string, Node>& supervisors,
    std::map<std::string, Tasks>& spoutTasks,
    std::map<std::string, Tasks>& boltTasks)
{
    if ( fieldDestinations.find(fieldPair) != fieldDestinations.end() ) {
        return fieldDestinations[fieldPair];
    }

    std::string boltName = fieldPair.first;
    std::string field = fieldPair.second;

    auto &network = topology->GetNetwork();
    int destIndex = rand() % boltTasks.size();
    int currentIndex = 0;
    int taskDestIndex = rand() % DEFAULT_BOLT_EXECUTOR_COUNT;

    std::string hostName;
    for ( auto& boltTasksPair : boltTasks ) {
        int taskIndex = 0;
        std::string supervisorName = boltTasksPair.first;
        std::string nextBoltName;

        for ( auto & taskName : boltTasksPair.second ) {
            if ( taskName == nextBoltName ) {
                if ( currentIndex == taskDestIndex ) {
                    destIndex = taskIndex;
                    hostName = supervisorName;
                    break;
                }

                currentIndex ++;
                taskIndex ++;
            }
        }
    }

    hurricane::base::Node node = supervisors[hostName];
    auto result = std::make_pair(node, destIndex);
    fieldDestinations.insert({ fieldPair, result });

    return result;
}

int main() {
    std::cerr << "Nimbus started" << std::endl;

    // 使用GetTopology从文件中装载拓扑结构,并将拓扑结构保存在topology变量里
    ITopology* topology = GetTopology();

    // 定义Manager的集合,这是一个映射表结构,每一个supervisors的机器名对应一个Node节点对象
    std::map<std::string, Node> supervisors;
    // 定义Spout任务,用于记录每个supervisors上消息源任务分配情况,如果对应的执行器没有任务运行,就用一个空字符串表示
    std::map<std::string, Tasks> spoutTasks;
    // 定义bolttasks变量,保存了supervisors上所有消息源任务的分配情况,同上
    std::map<std::string, Tasks> boltTasks;
    // fieldDestinations,其本身是一个映射表,映射表的键是一个键值对,值也是一个键值对,键对应了数据源或者数据处理单元的任务名称和字段名称,值则对应了supervisor节点和执行器编号
    std::map <std::pair <std::string, std::string> , 
        std::pair < Node, int >> fieldDestinations;

    // 定义NetListener对象,并监听NIMBUS_ADDRESS这个地址
    NetListener netListeer(NIMBUS_ADDRESS);
    // 该对象负责将网络消息转换成命令并转发到各个处理函数,属于上层接口
    CommandDispatcher dispatcher;
    // 这里使用oncommand来监听命令,这里监听的是command命令.同时用lambda表达式定义一个回调函数,用于处理join命令.
    // 该lambda表达式包含两个参数,一个是命令中附带的参数合集,属于variants类型,一个是src,表示命令源的tcp连接对象
    dispatcher
        .OnCommand(Command::Type::Join,
            [&](hurricane::base::Variants args, std::shared_ptr<meshy::TcpConnection> src) -> void {
        // 获取第一个参数,并将其转换为字符串,该参数是想要加入集群的Manager的主机名
        std::string supervisorName = args[0].GetStringValue();

        // Create supervisor node(节点名使用客户端传递过来的主机名,然后为其分配一个网络地址,准备与其通信)
        Node supervisor(supervisorName, SUPERVISOR_ADDRESSES.at(supervisorName));
        supervisor.SetStatus(Node::Status::Alived);
        supervisors[supervisorName] = supervisor;

        // Create empty tasks(创建空的任务列表,因为刚初始化完成的节点不会执行任何任务)
        spoutTasks[supervisorName] = Tasks(EXECUTOR_CAPACITY);
        boltTasks[supervisorName] = Tasks(EXECUTOR_CAPACITY);

        // 创建一个新的命令对象,作为该命令的返回值.返回值只有一个值,就是中央节点的主机名
        Command command(Command::Type::Response, {
            std::string("nimbus")
        });

        // 使用toDataPackage成员函数,将命令转换成用于序列化的数据包对象
        // 接着使用Serialize成员函数,将数据包的数据序列化成字节流,并将字节数组保存在commandBytes数组对象中
        ByteArray commandBytes = command.ToDataPackage().Serialize();
        src->Send(*(reinterpret_cast<meshy::ByteArray*>(&commandBytes)));// 将响应数据发回Manager(这里的supervisor),这样我们就完成了加入集群的响应

        if ( supervisors.size() == SUPERVISOR_ADDRESSES.size() ) {
            std::cout << "All supervisors started" << std::endl;
            dispatchTasks(supervisors, spoutTasks, boltTasks, topology);// 对新加入的节点进行任务分配.分配任务在新的节点上执行
        }
    })
        .OnCommand(Command::Type::Alive,
            [&](hurricane::base::Variants args, std::shared_ptr<meshy::TcpConnection> src) -> void {
        std::string supervisorName = args[0].GetStringValue();
        supervisors[supervisorName].Alive();

        Command command(Command::Type::Response, {
            std::string("nimbus")
        });

        ByteArray commandBytes = command.ToDataPackage().Serialize();
        src->Send(*(reinterpret_cast<meshy::ByteArray*>(&commandBytes)));
    })
        .OnCommand(Command::Type::RandomDestination,
            [&](hurricane::base::Variants args, std::shared_ptr<meshy::TcpConnection> src) -> void {
        std::string supervisorName = args[0].GetStringValue();
        std::string srcType = args[1].GetStringValue();
        int srcIndex = args[2].GetIntValue();

        if ( srcType == "spout" ) {
            std::string spoutName = spoutTasks[supervisorName][srcIndex];

            std::string nextBoltName;
            auto &network = topology->GetNetwork();
            int destIndex = rand() % boltTasks.size();
            int currentIndex = 0;
            for ( auto& taskName : network.at(spoutName) ) {
                if ( destIndex == currentIndex ) {
                    destIndex = currentIndex;
                }

                nextBoltName = taskName;
            }

            int taskDestIndex = rand() % DEFAULT_BOLT_EXECUTOR_COUNT;

            currentIndex = 0;
            destIndex = 0;

            std::string hostName;
            for ( auto& boltTasksPair : boltTasks ) {
                int taskIndex = 0;
                std::string supervisorName = boltTasksPair.first;
                for ( auto & taskName : boltTasksPair.second ) {
                    if ( taskName == nextBoltName ) {
                        if ( currentIndex == taskDestIndex ) {
                            destIndex = taskIndex;
                            hostName = supervisorName;
                            break;
                        }

                        currentIndex ++;
                        taskIndex ++;
                    }
                }
            }

            Command command(Command::Type::Response, {
                std::string("nimbus"), 
                supervisors[hostName].GetAddress().GetHost(),
                supervisors[hostName].GetAddress().GetPort(),
                destIndex
            });

            ByteArray commandBytes = command.ToDataPackage().Serialize();
            src->Send(*(reinterpret_cast<meshy::ByteArray*>(&commandBytes)));
        }
        else if ( srcType == "bolt" ) {
            std::string boltName = boltTasks[supervisorName][srcIndex];

            std::string nextBoltName;
            auto &network = topology->GetNetwork();
            int destIndex = rand() % boltTasks.size();
            int currentIndex = 0;
            for ( auto& taskName : network.at(boltName) ) {
                if ( destIndex == currentIndex ) {
                    destIndex = currentIndex;
                }

                nextBoltName = taskName;
            }

            int taskDestIndex = rand() % DEFAULT_BOLT_EXECUTOR_COUNT;

            currentIndex = 0;
            destIndex = 0;
            std::string hostName;
            for ( auto& boltTasksPair : boltTasks ) {
                int taskIndex = 0;
                std::string supervisorName = boltTasksPair.first;
                for ( auto & taskName : boltTasksPair.second ) {
                    if ( taskName == nextBoltName ) {
                        if ( currentIndex == taskDestIndex ) {
                            destIndex = taskIndex;
                            hostName = supervisorName;
                            break;
                        }

                        currentIndex ++;
                        taskIndex ++;
                    }
                }
            }

            Command command(Command::Type::Response, {
                std::string("nimbus"),
                supervisors[hostName].GetAddress().GetHost(),
                supervisors[hostName].GetAddress().GetPort(),
                destIndex
            });

            ByteArray commandBytes = command.ToDataPackage().Serialize();
            src->Send(*(reinterpret_cast<meshy::ByteArray*>(&commandBytes)));
        }
    })
    .OnCommand(Command::Type::GroupDestination,
        [&](hurricane::base::Variants args, std::shared_ptr<meshy::TcpConnection> src) -> void {
        std::string supervisorName = args[0].GetStringValue();
        std::string srcType = args[1].GetStringValue();
        int srcIndex = args[2].GetIntValue();
        int fieldIndex = args[3].GetIntValue();
        std::string field;

        if ( srcType == "spout" ) {
            std::string spoutName = spoutTasks[supervisorName][srcIndex];
            field = topology->GetSpouts().at(spoutName)->DeclareFields()[fieldIndex];
            auto result = GetFieldDestination(fieldDestinations,
                std::make_pair(spoutName, field), topology, supervisors, spoutTasks, boltTasks);


            Command command(Command::Type::Response, {
                std::string("nimbus"),
                result.first.GetAddress().GetHost(),
                result.first.GetAddress().GetPort(),
                result.second
            });

            ByteArray commandBytes = command.ToDataPackage().Serialize();
            src->Send(*(reinterpret_cast<meshy::ByteArray*>(&commandBytes)));
        }
        else if ( srcType == "bolt" ) {
            std::string boltName = boltTasks[supervisorName][srcIndex];
            std::string field = topology->GetBolts().at(boltName)->DeclareFields()[fieldIndex];
            auto result = GetFieldDestination(fieldDestinations,
                std::make_pair(boltName, field), topology, supervisors, spoutTasks, boltTasks);

            Command command(Command::Type::Response, {
                std::string("nimbus"),
                result.first.GetAddress().GetHost(),
                result.first.GetAddress().GetPort(),
                result.second
            });

            ByteArray commandBytes = command.ToDataPackage().Serialize();
            src->Send(*(reinterpret_cast<meshy::ByteArray*>(&commandBytes)));
        }

        if ( srcType == "spout" ) {
        }
        else if ( srcType == "bolt" ) {
            std::string boltName = boltTasks[supervisorName][srcIndex];

            std::string nextBoltName;
            auto &network = topology->GetNetwork();
            int destIndex = rand() % boltTasks.size();
            int currentIndex = 0;
            for ( auto& taskName : network.at(boltName) ) {
                if ( destIndex == currentIndex ) {
                    destIndex = currentIndex;
                }

                nextBoltName = taskName;
            }

            int taskDestIndex = rand() % DEFAULT_BOLT_EXECUTOR_COUNT;

            currentIndex = 0;
            destIndex = 0;
            std::string hostName;
            for ( auto& boltTasksPair : boltTasks ) {
                int taskIndex = 0;
                std::string supervisorName = boltTasksPair.first;
                for ( auto & taskName : boltTasksPair.second ) {
                    if ( taskName == nextBoltName ) {
                        if ( currentIndex == taskDestIndex ) {
                            destIndex = taskIndex;
                            hostName = supervisorName;
                            break;
                        }

                        currentIndex ++;
                        taskIndex ++;
                    }
                }
            }

            Command command(Command::Type::Response, {
                std::string("nimbus"),
                supervisors[hostName].GetAddress().GetHost(),
                supervisors[hostName].GetAddress().GetPort(),
                destIndex
            });

            ByteArray commandBytes = command.ToDataPackage().Serialize();
            src->Send(*(reinterpret_cast<meshy::ByteArray*>(&commandBytes)));
        }
    });
    
    // 这里是业务层以下的部分,NETlistener消息处理部分
    // 该网络通信的data事件,回调函数是一个Lambda表达式,该表达式1个参数是客户端的Tcp连接,第2个参数是数据缓冲区首地址,第三个参数是数据长度
    netListener.OnData([&](meshy::TcpStream* connection, 
            const char* buffer, int32_t size) -> void {
        // 利用收到的数据构建一个信息的字节数组对象,保存在receiveData中.
        ByteArray receivedData(buffer, size);
        DataPackage receivedPackage;
        receivedPackage.Deserialize(receivedData);
        
        Command command(receivedPackage);

        dispatcher.Dispatch(command);
    });

    netListener.StartListen();

    return 0;
}
