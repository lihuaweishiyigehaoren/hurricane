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

#include "hurricane/base/Fields.h"

namespace hurricane {
namespace base {

class ITask {
public:
    struct Strategy {
        enum Values {
            Global = 0,
            Random = 1,
            Group = 2
        };
    };

 	/*
    # 类型:虚析构函数
    # 作用1:虚函数才支持函数调用的动态绑定,根据实际对象的类型执行特定版本的函数(动态绑定)
    # 作用2:虚析构函数根据实际对象类型从该类型出发,直到其继承树的根部类型为止,依次调用所有的析构函数,
    若然没有virtual标记,销毁对象时,由于没有动态绑定,所有编译器不知道某个指针或引用指向的实际对象类型,可能导致
    实际对象的析构函数没有被执行(仅执行父类的析构函数)
    */
    virtual ~ITask() {}

	// 声明任务的字段名,每个任务都会输出一系列数据,Fields对象用来为这些数据命名
    virtual Fields DeclareFields() const = 0;

    Strategy::Values GetStrategy() const {
        return _strategy;
    }

    void SetStrategy(Strategy::Values strategy) {
        _strategy = strategy;
    }

private:
    Strategy::Values _strategy;
};

}
}
