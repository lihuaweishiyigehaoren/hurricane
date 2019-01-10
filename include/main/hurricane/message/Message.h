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

#include <cstdint>

namespace hurricane {

namespace message {
	// 所有的具体消息类型都应该继承自该消息类,并且不同的类型需要不同的消息类型,防止消息发生冲突
	// 除此之外,消息的具体实现和数据传输方式就交给具体实现去解决了
	class Message {
	public:

		// 该枚举类型定义了所有的消息类型,作为示例,我们定义了一个STOP消息
		// 并为消息赋予了一个数字0,作为消息的唯一代码
		struct Type {
			enum {
				Stop = 0
			};
		};

		Message(int32_t type) : _type(type) {
		}

		virtual ~Message() {
		}

		int32_t GetType() const {
			return _type;
		}

		void SetType(int32_t type) {
			_type = type;
		}

	private:
		int32_t _type;// 消息类型
	};
}

}