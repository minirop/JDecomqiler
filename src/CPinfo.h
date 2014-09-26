/*
JDecomqiler

Copyright (c) 2014 <Alexander Roper>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/
#ifndef CPINFO_H
#define CPINFO_H

#include <cstdint>

enum {
	CONSTANT_Utf8 = 1,
	CONSTANT_Integer = 3,
	CONSTANT_Float = 4,
	CONSTANT_Long = 5,
	CONSTANT_Double = 6,
	CONSTANT_Class = 7,
	CONSTANT_String = 8,
	CONSTANT_Fieldref = 9,
	CONSTANT_Methodref = 10,
	CONSTANT_InterfaceMethodref = 11,
	CONSTANT_NameAndType = 12,
	CONSTANT_MethodHandle = 15,
	CONSTANT_MethodType = 16,
	CONSTANT_InvokeDynamic = 18
};

struct CPinfo {
	std::uint8_t tag;
	union {
		struct {
			std::uint16_t name_index;
		} ClassInfo;
		struct {
			std::uint16_t class_index;
			std::uint16_t name_and_type_index;
		} RefInfo;
		struct {
			std::uint16_t string_index;
		} StringInfo;
		struct {
			std::uint32_t bytes;
		} IntegerInfo;
		struct {
			std::uint32_t bytes;
		} FloatInfo;
		struct {
			std::uint64_t bytes;
		} BigIntInfo;
		struct {
			std::uint64_t bytes;
		} DoubleInfo;
		struct {
			std::uint16_t name_index;
			std::uint16_t descriptor_index;
		} NameAndTypeInfo;
		struct {
			std::uint16_t length;
			char * bytes; // (1) find a better way
		} UTF8Info;
		struct {
			std::uint8_t reference_kind;
			std::uint16_t reference_index;
		} MethodHandleInfo;
		struct {
			std::uint16_t descriptor_index;
		} MethodTypeInfo;
		struct {
			std::uint16_t bootstrap_method_attr_index;
			std::uint16_t name_and_type_index;
		} InvokeDynamicInfo;
	};
};

#endif
