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
#include "hurricane/message/MessageLoop.h"

#include <thread>
#include <memory>
#include <vector>
#include <iostream>
#include <mutex>
#include <functional>

namespace hurricane {
namespace base {

template <class TaskType>
class Executor {
public:
    // status表示执行器的状态,某个执行器可能在执行任务,此时状态为Running ,否则为Stopping
    enum class Status {
        Stopping,
        Running
    };

    Executor() : _status(Status::Stopping) {
    }

    virtual ~Executor() {}

    // 负责启动任务,其实就是设置一下任务名,保存用户传递的任务,并创建一个新的线程,准备执行任务,入口为StartThread
    void StartTask(const std::string& taskName, TaskType* task) {
		_messageLoop.MessageMap(BoltMessage::MessageType::Data,
			this, &BoltMessageLoop::OnData);
        _taskName = taskName;
        _task = std::shared_ptr<TaskType>(task);

		_thread = std::thread(std::bind(&Executor::StartThread, this));
    }

    // 停止
    virtual void StopTask() {
		_messageLoop.Stop();
    }

    Status GetStatus() const {
        return _status;
    }

    const std::string& GetTaskName() const {
        return _taskName;
    }

protected:
	virtual void OnCreate() = 0;
	virtual void OnStop() = 0;
    std::shared_ptr<TaskType> _task;
	hurricane::message::MessageLoop _messageLoop;

private:
    /*
    #执行过程
    1、设置任务状态
    2、调用oncreate初始化任务执行器
    3、启动消息队列,消息队列结束执行OnStop停止执行器
    4、等待Manager的下一次调度
    */
    void StartThread() {
		_status = Status::Running;

		OnCreate();
		_messageLoop.Run();
		OnStop();

        _status = Status::Stopping;
    }

    std::thread _thread;
	Status _status;
    std::string _taskName;
};

}
}
