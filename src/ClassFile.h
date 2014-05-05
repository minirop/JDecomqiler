/*
JDecomqiler

Copyright (c) 2011, 2014 <Alexander Roper>

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
#ifndef CLASSFILE_H
#define CLASSFILE_H

#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "ClassOutput.h"

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
	CONSTANT_NameAndType = 12
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
	};
};

class StreamReader
{
public:
	StreamReader();
	void setStream(std::ifstream * stream);
	void readRawData(char * s, std::streamsize length);
	int getPos();
	
	StreamReader& operator>>(std::int8_t & i);
	StreamReader& operator>>(std::int16_t & i);
	StreamReader& operator>>(std::int32_t & i);
	StreamReader& operator>>(std::int64_t & i);
	StreamReader& operator>>(std::uint8_t & u);
	StreamReader& operator>>(std::uint16_t & u);
	StreamReader& operator>>(std::uint32_t & u);
	StreamReader& operator>>(std::uint64_t & u);
	
private:
	std::ifstream * buffer;
};

class ClassFile
{
public:
	ClassFile(std::string filename);
	~ClassFile();
	
	void generate();

private:
	ClassOutput output;
	std::vector<CPinfo> constant_pool;
	
	std::vector<char *> toDelete; // find a better way (see (1))
	
	StreamReader stream;
	
	// functions
	std::tuple<std::string, std::string> parseAttribute();
	bool parseConstant();
	FieldOutput parseField();
	std::string parseInterface();
	MethodOutput parseMethod();
	
	std::string getName(uint16_t index);
	std::vector<std::string> parseSignature(std::string signature);
	std::string parseType(std::string signature, int & i);
	std::string checkClassName(std::string classname);
	char letterFromType(std::string type);
	std::string typeFromInt(int typeInt);
	std::string removeArray(std::string className);
};

#endif
