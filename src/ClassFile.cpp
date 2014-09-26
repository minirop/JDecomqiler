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
#include "ClassFile.h"
#include "defines.h"
#include <iostream>

using namespace std;

StreamReader::StreamReader()
{
}

void StreamReader::setStream(std::ifstream * stream)
{
	buffer = stream;
}

void StreamReader::readRawData(char * s, std::streamsize length)
{
	buffer->read(s, length);
}

int StreamReader::getPos()
{
	return buffer->tellg();
}

#define READ_BUFF(size) \
	unsigned char s[size] = {0}; \
	buffer->read(reinterpret_cast<char *>(s), size);

StreamReader& StreamReader::operator>>(std::int8_t & i)
{
	READ_BUFF(1)
	i = static_cast<std::int8_t>(s[0]);
	return *this;
}

StreamReader& StreamReader::operator>>(std::int16_t & i)
{
	READ_BUFF(2)
	i = static_cast<std::int16_t>(s[0] << 8 | s[1]);
	return *this;
}

StreamReader& StreamReader::operator>>(std::int32_t & i)
{
	READ_BUFF(4)
	i = static_cast<std::int32_t>(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
	return *this;
}

StreamReader& StreamReader::operator>>(std::int64_t & i)
{
	READ_BUFF(8)
	i = 0LL;
	return *this;
}

StreamReader& StreamReader::operator>>(std::uint8_t & u)
{
	READ_BUFF(1)
	u = static_cast<std::uint8_t>(s[0]);
	return *this;
}

StreamReader& StreamReader::operator>>(std::uint16_t & u)
{
	READ_BUFF(2)
	u = static_cast<std::uint16_t>(s[0] << 8 | s[1]);
	return *this;
}

StreamReader& StreamReader::operator>>(std::uint32_t & u)
{
	READ_BUFF(4)
	u = static_cast<std::uint32_t>(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
	return *this;
}

StreamReader& StreamReader::operator>>(std::uint64_t & u)
{
	READ_BUFF(8)
	u = 0ULL;
	return *this;
}

ClassFile::ClassFile(std::string filename)
{
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if(!file.is_open())
		return;
	
	stream.setStream(&file);
	
	std::uint32_t magic;
	stream >> magic;
	if(magic != 0xcafebabe)
	{
		cout << "magic number is " << std::hex << magic << endl;
		return;
	}
	
	std::uint16_t major, minor;
	stream >> minor >> major;
	cout << "JAVA " << major << "." << minor << endl;
	
	std::uint16_t constant_pool_count;
	stream >> constant_pool_count;
	cout << constant_pool_count << " constants" << endl;
	
	constant_pool.push_back(CPinfo()); // index 0 is invalid
	for(std::size_t i = 1;i < constant_pool_count;i++)
	{
		if(parseConstant()) // return true if double or bigint
		{
			constant_pool.push_back(CPinfo()); // double and bigint use 2 indexes
			i++;
		}
	}
	
	std::uint16_t access_flags, this_class, super_class;
	stream >> access_flags >> this_class >> super_class;
	
	if(access_flags & ACC_PUBLIC)
		output.isPublic = true;
	if(access_flags & ACC_FINAL)
		output.isFinal = true;
	if(access_flags & ACC_SUPER)
		cout << "is super, ignored." << endl;
	if(access_flags & ACC_INTERFACE)
		output.isInterface = true;
	if(access_flags & ACC_ABSTRACT)
		output.isAbstract = true;
	if(access_flags & ACC_ANNOTATION)
		output.isAnnotation = true;
	if(access_flags & ACC_ENUM)
		output.isEnum = true;
	if(access_flags & (~0x0631))
		cout << "ERROR: unrecognized flag(s)" << endl;
	
	output.name = getName(constant_pool[this_class].ClassInfo.name_index);
	output.extends = checkClassName(getName(constant_pool[super_class].ClassInfo.name_index));
	
	std::uint16_t interfaces_count;
	stream >> interfaces_count;
	cout << interfaces_count << " interfaces" << endl;
	for(std::uint16_t i = 0;i < interfaces_count;i++)
	{
		output.interfaces.push_back(parseInterface());
	}
	
	std::uint16_t fields_count;
	stream >> fields_count;
	cout << fields_count << " fields" << endl;
	for(std::uint16_t i = 0;i < fields_count;i++)
	{
		output.fields.push_back(parseField());
	}
	
	std::uint16_t methods_count;
	stream >> methods_count;
	cout << methods_count << " methods" << endl;
	for(std::uint16_t i = 0;i < methods_count;i++)
	{
		output.methods.push_back(parseMethod());
	}
	
	std::uint16_t attributes_count;
	stream >> attributes_count;
	cout << attributes_count << " attributes" << endl;
	for(std::uint16_t i = 0;i < attributes_count;i++)
	{
		parseAttribute();
	}
	
	file.close();
}

ClassFile::~ClassFile()
{
	for(auto str : toDelete)
	{
		delete[] str;
	}
}

std::tuple<std::string, std::string> ClassFile::parseAttribute()
{
	std::uint16_t attribute_name_index;
	stream >> attribute_name_index;
	
	std::string name = getName(attribute_name_index);
	
	std::uint32_t length;
	stream >> length;
	if(length > 512)
	{
		cerr << "attribute " << name << " has more than 512 bytes of data" << endl;
		exit(1);
	}
	
	char tmp[512] = {0};
	stream.readRawData(tmp, length);
	std::string data(tmp, length);
	
	return std::make_tuple(name, data);
}

bool ClassFile::parseConstant()
{
	bool isDoubleSize = false;
	
	CPinfo info;
	
	stream >> info.tag;
	
	switch(info.tag)
	{
		case CONSTANT_Utf8:
			{
				std::uint16_t length;
				stream >> length;
				char * string = new char[length+1];
				stream.readRawData(string, length);
				string[length] = 0;
				info.UTF8Info.bytes = string;
				toDelete.push_back(string);
			}
			break;
		case CONSTANT_Integer:
			stream >> info.IntegerInfo.bytes;
			break;
		case CONSTANT_Float:
			stream >> info.FloatInfo.bytes;
			break;
		case CONSTANT_Long:
			stream >> info.BigIntInfo.bytes;
			isDoubleSize = true;
			break;
		case CONSTANT_Double:
			stream >> info.DoubleInfo.bytes;
			isDoubleSize = true;
			break;
		case CONSTANT_Class:
			stream >> info.ClassInfo.name_index;
			break;
		case CONSTANT_String:
			stream >> info.StringInfo.string_index;
			break;
		case CONSTANT_Fieldref:
			stream >> info.RefInfo.class_index;
			stream >> info.RefInfo.name_and_type_index;
			break;
		case CONSTANT_Methodref:
			stream >> info.RefInfo.class_index;
			stream >> info.RefInfo.name_and_type_index;
			break;
		case CONSTANT_InterfaceMethodref:
			stream >> info.RefInfo.class_index;
			stream >> info.RefInfo.name_and_type_index;
			break;
		case CONSTANT_NameAndType:
			stream >> info.NameAndTypeInfo.name_index;
			stream >> info.NameAndTypeInfo.descriptor_index;
			break;
		case CONSTANT_MethodHandle:
			stream >> info.MethodHandleInfo.reference_kind;
			stream >> info.MethodHandleInfo.reference_index;
			break;
		case CONSTANT_MethodType:
			stream >> info.MethodTypeInfo.descriptor_index;
			break;
		case CONSTANT_InvokeDynamic:
			stream >> info.InvokeDynamicInfo.bootstrap_method_attr_index;
			stream >> info.InvokeDynamicInfo.name_and_type_index;
			break;
		default:
			cerr << "ERROR: unrecognize TAG" << endl;
			exit(1);
	}
	
	constant_pool.push_back(info);
	
	return isDoubleSize;
}

FieldOutput ClassFile::parseField()
{
	FieldOutput field;
	
	std::uint16_t access_flags, name_index, descriptor_index;
	stream >> access_flags >> name_index >> descriptor_index;
	
	field.name = getName(name_index);
	int i = 0;
	field.type = parseType(getName(descriptor_index), i);
	
	if(access_flags & ACC_PUBLIC)
		field.isPublic = true;
	if(access_flags & ACC_PRIVATE)
		field.isPrivate = true;
	if(access_flags & ACC_PROTECTED)
		field.isProtected = true;
	if(access_flags & ACC_STATIC)
		field.isStatic = true;
	if(access_flags & ACC_FINAL)
		field.isFinal = true;
	if(access_flags & ACC_VOLATILE)
		field.isVolatile = true;
	if(access_flags & ACC_TRANSIENT)
		field.isTransient = true;
	if(access_flags & ACC_SYNTHETIC)
		cout << "Declared synthetic; not present in the source code." << endl;
	if(access_flags & ACC_ENUM)
		cout << "is part of an enum." << endl;
	if(access_flags & ~ACC_FIELD_MASK)
		cerr << "ERROR: unrecognized flag(s)" << endl;
	
	std::uint16_t attributes_count;
	stream >> attributes_count;
	cout << "- " << attributes_count << " attributes" << endl;
	for(std::uint16_t i = 0;i < attributes_count;i++)
	{
		field.attributes.push_back(parseAttribute());
	}
	
	return field;
}

std::string ClassFile::parseInterface()
{
	std::uint16_t name_index;
	std::string interfaceName;
	
	stream >> name_index;
	CPinfo *data = &constant_pool[name_index];
	if(data->tag == CONSTANT_Class)
	{
		interfaceName = getName(data->ClassInfo.name_index);
	}
	else
	{
		cerr << "ERROR: index " << name_index << " in not a class" << endl;
	}
	
	return interfaceName;
}

MethodOutput ClassFile::parseMethod()
{
	MethodOutput method;
	
	std::uint16_t access_flags, name_index, descriptor_index;
	stream >> access_flags >> name_index >> descriptor_index;
	method.name = getName(name_index);
	
	if(access_flags & ACC_PUBLIC)
		method.isPublic = true;
	if(access_flags & ACC_PRIVATE)
		method.isPrivate = true;
	if(access_flags & ACC_PROTECTED)
		method.isProtected = true;
	if(access_flags & ACC_STATIC)
		method.isStatic = true;
	if(access_flags & ACC_FINAL)
		method.isFinal = true;
	if(access_flags & ACC_SYNCHRONIZED)
		method.isSynchronized = true;
	if(access_flags & ACC_BRIDGE)
		method.isBridge = true;
	if(access_flags & ACC_VARARGS)
		method.isVarargs = true;
	if(access_flags & ACC_NATIVE)
		method.isNative = true;
	if(access_flags & ACC_ABSTRACT)
		method.isAbstract = true;
	if(access_flags & ACC_STRICT)
		method.isStrict = true;
	if(access_flags & ACC_SYNTHETIC)
		cout << "Declared synthetic; not present in the source code." << endl;
	if(access_flags & ~ACC_METHOD_MASK)
		cerr << "ERROR: unrecognized flag(s)" << endl;
	
	method.parametersType = parseSignature(getName(descriptor_index));
	method.returnType = method.parametersType.back();
	method.parametersType.pop_back();
	
	std::uint16_t attributes_count;
	stream >> attributes_count;
	for(std::uint16_t i = 0;i < attributes_count;i++)
	{
		method.attributes.push_back(parseAttribute());
	}
	
	return method;
}

void ClassFile::generate()
{
	std::ofstream file("output.java");
	if(!file.is_open())
		return;
	
	output.generate(file);
}

