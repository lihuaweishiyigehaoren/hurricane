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

#include <string>
#include <exception>
#include <cstdint>
#include <vector>
#include <initializer_list>
#include <iostream>

#ifdef WIN32
#define NOEXCEPT
#else
#define NOEXCEPT noexcept
#endif // NOEXCEPT

namespace hurricane {
    namespace base {

        class Variant;
		// 标准异常类型
        class TypeMismatchException : std::exception {
        public:
            TypeMismatchException(const std::string& message) :
                _message(message) {}

			// 重载的父类的虚函数,系统用这个函数获取异常的文本消息.我们将构造函数的消息直接返回
            // noexcept 表示该函数不会抛出异常,这样有利于编译器进行函数调用优化
            // override 表示该函数是覆盖了一个父类的虚函数,如果不是覆盖了一个函数就会报错
            const char* what() const NOEXCEPT override{
                return _message.c_str();
            }

        private:
            std::string _message;// 异常消息
        };

		// 该类型表示一个可以存储任意类型的值
        class Value {
        public:

			// 枚举类,C++新加入特性,和传统的枚举相比,枚举类会使用枚举类类型名称来做命名空间越约束
            // 比如我们想要引用Type中名为Boolean的值,需要使用Value::Type::Boolean,而不能使用Value::Type
            // 枚举类不能和整数值之间进行任何类型转换,这样可以确保代码更加规范
            enum class Type {
                Invalid,
                Boolean,
                Character,
                Int8,
                Int16,
                Int32,
                Int64,
                Float,
                Double,
                String
            };

			// 联合体
            // 联合体的优点在于所有的值公用一片存储空间,因此不会引起额外的空间存储消耗
            // 概念:联合是一种变量,他可以在不同时间内维持不同类型和不同长度的对象,它提供了在单个存储区域中操作不同类型数据的方法,
            // 而无需在程序中存放与机器有关的信息
            // 联合是一种形式特殊的结构变量. 和结构一样,对联合施加的操作只能是存取成员和取其地址,不能把联合联合作为参数传递给函数,也不能由函数返回联合
            union InnerValue {
                bool booleanValue;
                char characterValue;
                int8_t int8Value;
                int16_t int16Value;
                int32_t int32Value;
                int64_t int64Value;
                float floatValue;
                double doubleValue;
            };

			// 以下函数都是用来:从一个普通的值转换成Value,这里我们支持前文体积的所有类型
            Value() : _type(Type::Invalid) {
            }

            Value(bool value) : _type(Type::Boolean) {
                _value.booleanValue = value;
            }

            Value(char value) : _type(Type::Character) {
                _value.characterValue = value;
            }

            Value(int8_t value) : _type(Type::Int8) {
                _value.int8Value = value;
            }

            Value(int16_t value) : _type(Type::Int16) {
                _value.int16Value = value;
            }

            Value(int32_t value) : _type(Type::Int32) {
                _value.int32Value = value;
            }

            Value(int64_t value) : _type(Type::Int64) {
                _value.int64Value = value;
            }

            Value(float value) : _type(Type::Float) {
                _value.floatValue = value;
            }

            Value(double value) : _type(Type::Double) {
                _value.doubleValue = value;
            }

            Value(const std::string& value) : _type(Type::String) {
                _stringValue = value;
            }

            Value(const char* value) : Value(std::string(value)) {
            }

			// 转换函数,可以将Value的值转换为实际的值,如果值的类型不匹配,则直接抛出TypeMismatchException
            bool ToBoolean() const {
                if ( _type != Type::Boolean ) {
                    throw TypeMismatchException("The type of value is not boolean");
                }
            }

            int8_t ToInt8() const {
                if ( _type != Type::Int8 ) {
                    throw TypeMismatchException("The type of value is not int8");
                }

                return _value.int8Value;
            }

            int16_t ToInt16() const {
                if ( _type != Type::Int16 ) {
                    throw TypeMismatchException("The type of value is not int16");
                }

                return _value.int16Value;
            }

            int32_t ToInt32() const {
                if ( _type != Type::Int32 ) {
                    throw TypeMismatchException("The type of value is not int32");
                }

                return _value.int32Value;
            }

            int64_t ToInt64() const {
                if ( _type != Type::Int64 ) {
                    throw TypeMismatchException("The type of value is not int64");
                }

                return _value.int64Value;
            }

            char ToCharacter() const {
                if ( _type != Type::Character ) {
                    throw TypeMismatchException("The type of value is not character");
                }

                return _value.characterValue;
            }

            const std::string& ToString() const {
                if ( _type != Type::String ) {
                    throw TypeMismatchException("The type of value is not string");
                }

                return _stringValue;
            }

            Variant ToVariant() const;
            static Value FromVariant(const Variant& variant);

        private:
            Type _type;
            InnerValue _value;
            std::string _stringValue;// 将字符串类型分离出来的原因-并不是所有的编译器都支持讲一个复杂的POD对象放在联合体中,这样更有利于可移植性
        };
		
		// 该类型的接口应该支持任意基础类型和值类型之间的转换,因此这方面的接口略为复杂,而元祖就是一个由值组成的有序序列.定义如下
        class Values : public std::vector<Value> {
        public:
            Values() = default;
            Values(std::initializer_list<Value> values) : std::vector<Value>(values) {
            }

			// 索引操作符,和普通向量一模一样
            Value& operator[](size_t index) {
                return std::vector<Value>::operator[](index);
            }

            const Value& operator[](size_t index) const {
                return std::vector<Value>::operator[](index);
            }
        };

    }
}
