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
#include <algorithm>

using std::cout;
using std::cerr;
using std::endl;

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
	
	if(access_flags & 0x0001)
		output.isPublic = true;
	if(access_flags & 0x0010)
		output.isFinal = true;
	if(access_flags & 0x0020)
		cout << "is super" << endl;
	if(access_flags & 0x0200)
		output.isInterface = true;
	if(access_flags & 0x0400)
		output.isAbstract = true;
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

std::vector<std::string> ClassFile::parseSignature(std::string signature)
{
	std::vector<std::string> params;
	
	int i = 0;
	if(signature[i] == '(')
	{
		i++;
		
		int next_is_array = 0;
		std::string type;
		while(signature[i] != ')')
		{
			if(signature[i] == '[')
			{
				next_is_array++;
			}
			else
			{
				type = parseType(signature, i);
			}
			
			if(type.length() > 0)
			{
				std::string tmp = type;
				while(next_is_array > 0)
				{
					tmp += "[]";
					next_is_array--;
				}
				params.push_back(tmp);
				tmp.clear();
			}
			
			i++;
		}
		i++; // skip the ')'
	}
	
	params.push_back(parseType(signature, i));
	
	return params;
}

std::string ClassFile::parseType(std::string signature, int & i)
{
	std::string tmp;
	switch(signature[i])
	{
		case 'V':
			tmp = "void";
			break;
		case 'I':
			tmp = "int";
			break;
		case 'L':
			do
			{
				i++;
				tmp += signature[i];
			} while(signature[i] != ';');
			tmp.pop_back();
			break;
		default:
			cerr << "unrecognized parameter '" << signature[i] << "' type in signature" << endl;
			exit(1);
	}
	
	tmp = checkClassName(tmp);
	
	return tmp;
}

std::string ClassFile::getName(std::uint16_t index)
{
	std::string ret_string("*ERROR*");
	
	CPinfo *data = &constant_pool[index];
	if(data->tag == CONSTANT_Utf8)
	{
		ret_string = std::string(data->UTF8Info.bytes);
	}
	
	return ret_string;
}

#define W(c) file << c

#define STORE(type, value, index) \
	if(!store[index]) \
	{ \
		W(type); \
		W(" "); \
		store[index] = true; \
	} \
	W(letterFromType(type)); \
	W(index); \
	W(" = "); \
	W(value); \
	W(";\n"); \
	jvm_stack.pop_back();

void ClassFile::generate()
{
	std::ofstream file("output.java");
	if(!file.is_open())
		return;
	
	if(output.isPublic)
		W("public ");
	if(output.isAbstract)
		W("abstract ");
	if(output.isFinal)
		W("final ");
	if(output.isInterface)
		W("interface ");
	else
		W("class ");
	
	W(output.name);
	if(output.extends != "Object")
	{
		W(" extends ");
		W(output.extends);
	}
	if(output.interfaces.size() > 0)
	{
		W(" implements ");
		W(output.interfaces[0]);
		for(std::size_t i = 1;i < output.interfaces.size();i++)
		{
			W(", ");
			W(output.interfaces[i]);
		}
	}
	W(" {\n");
	
	for(MethodOutput m : output.methods)
	{
		bool isCtor = false;
		bool skip_return = false;
		
		if(m.isPublic)
			W("public ");
		if(m.isProtected)
			W("protected ");
		if(m.isPrivate)
			W("private ");
		if(m.isAbstract)
			W("abstract ");
		if(m.isFinal)
			W("final ");
		if(m.isStatic)
			W("static ");
		
		if(m.name == "<clinit>")
		{
			skip_return = true; // java bytecode have "return" at the end of static initialization blocks
		}
		else
		{
			if(m.name == "<init>")
			{
				W(output.name);
				isCtor = true;
			}
			else
			{
				W(m.returnType);
				W(" ");
				W(m.name);
				
			}
			W("(");
			int pos = (m.isStatic ? 0 : 1);
			for(std::string str : m.parametersType)
			{
				if(pos > 1)
					W(", ");
				W(str);
				W(" ");
				W(letterFromType(str));
				W(pos);
				pos++;
			}
			W(") ");
		}
		W("{\n");
		
		for(std::size_t i = 0;i < m.attributes.size();i++)
		{
			std::tuple<std::string, std::string> a = m.attributes[i];
			
			if(std::get<0>(a) == "Code")
			{
				const std::string & ref = std::get<1>(a);
				cout << "DEBUG REF SIZE: " << ref.size() << endl;
				int zz = 0;
				
				std::vector<std::string> jvm_stack;
				
				W("/*\n");
				
				W("stack size: ");
				unsigned char t1 = ref[zz++];
				unsigned char t2 = ref[zz++];
				unsigned int stack = ((t1 << 8) + t2);
				W(stack);
				
				W("\nhow many locals: ");
				t1 = ref[zz++];
				t2 = ref[zz++];
				unsigned int locals = ((t1 << 8) + t2);
				W(locals);
				std::vector<std::int32_t> localsVar(locals, 0);
				
				W("\ncode size: ");
				t1 = ref[zz++];
				t2 = ref[zz++];
				unsigned char t3 = ref[zz++];
				unsigned char t4 = ref[zz++];
				unsigned int code_size = ((t1 << 24) + (t2 << 16) + (t3 << 8) + t4);
				W(code_size);
				W("\n");
				W("*/\n");
				
				std::vector<bool> store(locals, false);
				std::size_t paramCount = m.parametersType.size();
				for(std::size_t storei = 0;storei < paramCount;storei++)
				{
					store[storei] = true;
				}
				
				std::vector<std::string> retNames;
				std::vector<std::string> tmpNames;
				std::map<std::string, std::string> varTypes;
				std::map<int, std::vector<std::string>> jumpTargets;
				
				int end = code_size + 8;
				for(int opcodePos = 0;zz < end;zz++)
				{
					opcodePos = zz - 8;
					if(jumpTargets.find(opcodePos) != jumpTargets.end())
					{
						for(auto & str : jumpTargets[opcodePos])
						{
							W(str);
						}
						
						jumpTargets.erase(opcodePos);
					}
					
					unsigned char c = ref[zz];
					switch(c)
					{
						case 0x00: // nop
							// skip
							break;
						case 0x01: // aconst_null
							jvm_stack.push_back("null");
							break;
						case 0x02: // iconst_m1
							jvm_stack.push_back("-1");
							break;
						case 0x03: // iconst_0
							jvm_stack.push_back("0");
							break;
						case 0x04: // iconst_1
							jvm_stack.push_back("1");
							break;
						case 0x05: // iconst_2
							jvm_stack.push_back("2");
							break;
						case 0x06: // iconst_3
							jvm_stack.push_back("3");
							break;
						case 0x07: // iconst_4
							jvm_stack.push_back("4");
							break;
						case 0x08: // iconst_5
							jvm_stack.push_back("5");
							break;
						case 0x09: // lconst_0
							jvm_stack.push_back("0L");
							break;
						case 0x0a: // lconst_1
							jvm_stack.push_back("1L");
							break;
						case 0x0b: // fconst_0
							jvm_stack.push_back("0.0f");
							break;
						case 0x0c: // fconst_1
							jvm_stack.push_back("1.0f");
							break;
						case 0x0d: // fconst_2
							jvm_stack.push_back("2.0f");
							break;
						case 0x0e: // dconst_0
							jvm_stack.push_back("0.0");
							break;
						case 0x0f: // dconst_1
							jvm_stack.push_back("1.0");
							break;
						case 0x10: // bipush
							{
								int idx = ref[++zz];
								jvm_stack.push_back(std::to_string(idx));
							}
							break;
						case 0x11: // sipush
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								jvm_stack.push_back(std::to_string(idx));
							}
							break;
						case 0x12: // ldc
							{
								int idx = ref[++zz];
								switch(constant_pool[idx].tag)
								{
									case CONSTANT_String:
										{
											std::string str = "\""+getName(constant_pool[idx].StringInfo.string_index)+"\"";
											varTypes[str] = "String";
											jvm_stack.push_back(str);
										}
										break;
									default:
										cerr << "0x12: unrecognized tag " << std::hex << static_cast<int>(constant_pool[idx].tag) << endl;
										exit(1);
								}
							}
							break;
						case 0x13: // ldc_w
						case 0x14: // ldc2_w
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								switch(constant_pool[idx].tag)
								{
									case CONSTANT_String:
										jvm_stack.push_back("\""+getName(constant_pool[idx].StringInfo.string_index)+"\"");
										break;
									default:
										cerr << std::hex << static_cast<int>(c) << ": unrecognized tag " << static_cast<int>(constant_pool[idx].tag) << endl;
										exit(1);
								}
							}
							break;
						case 0x15: // iload
							{
								int idx = ref[++zz];
								jvm_stack.push_back(std::string("i") + std::to_string(idx));
							}
							break;
						case 0x16: // lload
							{
								int idx = ref[++zz];
								jvm_stack.push_back(std::string("l") + std::to_string(idx));
							}
							break;
						case 0x17: // fload
							{
								int idx = ref[++zz];
								jvm_stack.push_back(std::string("f") + std::to_string(idx));
							}
							break;
						case 0x18: // dload
							{
								int idx = ref[++zz];
								jvm_stack.push_back(std::string("d") + std::to_string(idx));
							}
							break;
						case 0x19: // aload
							{
								int idx = ref[++zz];
								jvm_stack.push_back(std::string("a") + std::to_string(idx));
							}
							break;
						case 0x1a: // iload_0
							jvm_stack.push_back("i0");
							break;
						case 0x1b: // iload_1
							jvm_stack.push_back("i1");
							break;
						case 0x1c: // iload_2
							jvm_stack.push_back("i2");
							break;
						case 0x1d: // iload_3
							jvm_stack.push_back("i3");
							break;
						case 0x1e: // lload_0
							jvm_stack.push_back("l0");
							break;
						case 0x1f: // lload_1
							jvm_stack.push_back("l1");
							break;
						case 0x20: // lload_2
							jvm_stack.push_back("l2");
							break;
						case 0x21: // lload_3
							jvm_stack.push_back("l3");
							break;
						case 0x22: // fload_0
							jvm_stack.push_back("f0");
							break;
						case 0x23: // fload_1
							jvm_stack.push_back("f1");
							break;
						case 0x24: // fload_2
							jvm_stack.push_back("f2");
							break;
						case 0x25: // fload_3
							jvm_stack.push_back("f3");
							break;
						case 0x26: // dload_0
							jvm_stack.push_back("d0");
							break;
						case 0x27: // dload_1
							jvm_stack.push_back("d1");
							break;
						case 0x28: // dload_2
							jvm_stack.push_back("d2");
							break;
						case 0x29: // dload_3
							jvm_stack.push_back("d3");
							break;
						case 0x2a: // aload_0
							if(m.isStatic)
								jvm_stack.push_back("a0");
							else
								jvm_stack.push_back("this");
							break;
						case 0x2b: // aload_1
							jvm_stack.push_back("a1");
							break;
						case 0x2c: // aload_2
							jvm_stack.push_back("a2");
							break;
						case 0x2d: // aload_3
							jvm_stack.push_back("a3");
							break;
						case 0x2e: // iaload
						case 0x2f: // laload
						case 0x30: // faload
						case 0x31: // daload
						case 0x32: // aaload
						case 0x33: // baload
						case 0x34: // caload
						case 0x35: // saload
							{
								std::string index = jvm_stack.back();
								jvm_stack.pop_back();
								std::string arr = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(arr + "[" + index + "]");
							}
							break;
						case 0x36: // istore
							{
								int index = ref[++zz];
								STORE("int", jvm_stack.back(), index)
							}
							break;
						case 0x37: // lstore
							{
								int index = ref[++zz];
								STORE("long", jvm_stack.back(), index)
							}
							break;
						case 0x38: // fstore
							{
								int index = ref[++zz];
								STORE("float", jvm_stack.back(), index)
							}
							break;
						case 0x39: // dstore
							{
								int index = ref[++zz];
								STORE("double", jvm_stack.back(), index)
							}
							break;
						case 0x3a: // astore
							{
								int index = ref[++zz];
								std::string varName = jvm_stack.back();
								STORE(varTypes[varName], varName, index)
								// STORE("Object", varName, index)
								// varTypes["a" + std::to_string(index)] = varTypes[varName];
							}
							break;
						case 0x3b: // istore_0
							STORE("int", jvm_stack.back(), 0)
							break;
						case 0x3c: // istore_1
							STORE("int", jvm_stack.back(), 1)
							break;
						case 0x3d: // istore_2
							STORE("int", jvm_stack.back(), 2)
							break;
						case 0x3e: // istore_3
							STORE("int", jvm_stack.back(), 3)
							break;
						case 0x3f: // lstore_0
							STORE("long", jvm_stack.back(), 0)
							break;
						case 0x40: // lstore_1
							STORE("long", jvm_stack.back(), 1)
							break;
						case 0x41: // lstore_2
							STORE("long", jvm_stack.back(), 2)
							break;
						case 0x42: // lstore_3
							STORE("long", jvm_stack.back(), 3)
							break;
						case 0x43: // fstore_0
							STORE("float", jvm_stack.back(), 0)
							break;
						case 0x44: // fstore_1
							STORE("float", jvm_stack.back(), 1)
							break;
						case 0x45: // fstore_2
							STORE("float", jvm_stack.back(), 2)
							break;
						case 0x46: // fstore_3
							STORE("float", jvm_stack.back(), 3)
							break;
						case 0x47: // dstore_0
							STORE("double", jvm_stack.back(), 0)
							break;
						case 0x48: // dstore_1
							STORE("double", jvm_stack.back(), 1)
							break;
						case 0x49: // dstore_2
							STORE("double", jvm_stack.back(), 2)
							break;
						case 0x4a: // dstore_3
							STORE("double", jvm_stack.back(), 3)
							break;
						case 0x4b: // astore_0
							{
								std::string varName = jvm_stack.back();
								STORE(varTypes[varName], jvm_stack.back(), 0)
								// STORE("Object", jvm_stack.back(), 0)
								// varTypes["a0"] = varTypes[varName];
							}
							break;
						case 0x4c: // astore_1
							{
								std::string varName = jvm_stack.back();
								STORE(varTypes[varName], jvm_stack.back(), 1)
								// STORE("Object", jvm_stack.back(), 1)
								// varTypes["a1"] = varTypes[varName];
							}
							break;
						case 0x4d: // astore_2
							{
								std::string varName = jvm_stack.back();
								STORE(varTypes[varName], jvm_stack.back(), 2)
								// STORE("Object", jvm_stack.back(), 2)
								// varTypes["a2"] = varTypes[varName];
							}
							break;
						case 0x4e: // astore_3
							{
								std::string varName = jvm_stack.back();
								STORE(varTypes[varName], jvm_stack.back(), 3)
								// STORE("Object", jvm_stack.back(), 3)
								// varTypes["a3"] = varTypes[varName];
							}
							break;
						case 0x4f: // iastore
						case 0x50: // lastore
						case 0x51: // fastore
						case 0x52: // dastore
						case 0x53: // aastore
						case 0x54: // bastore
						case 0x55: // castore
						case 0x56: // sastore
							{
								std::string value = jvm_stack.back();
								jvm_stack.pop_back();
								std::string index = jvm_stack.back();
								jvm_stack.pop_back();
								std::string arr = jvm_stack.back();
								jvm_stack.pop_back();
								W(arr + "[" + index + "] = " + value + ";\n");
							}
							break;
						case 0x57: // pop
						case 0x58: // pop2 -- since we don't treat double as 2 values
							jvm_stack.pop_back();
							break;
						case 0x59: // dup
							jvm_stack.push_back(jvm_stack.back());
							break;
						case 0x5a: // dup_x1
							// TODO
							break;
						case 0x5b: // dup_x2
							// TODO
							break;
						case 0x5c: // dup2
							// TODO
							break;
						case 0x5d: // dup2_x1
							// TODO
							break;
						case 0x5e: // dup2_x2
							// TODO
							break;
						case 0x5f: // swap
							{
								std::string first = jvm_stack.back();
								jvm_stack.pop_back();
								std::string second = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(first);
								jvm_stack.push_back(second);
							}
							break;
						case 0x60: // iadd
						case 0x61: // ladd
						case 0x62: // fadd
						case 0x63: // dadd
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " + " + y);
							}
							break;
						case 0x64: // isub
						case 0x65: // lsub
						case 0x66: // fsub
						case 0x67: // dsub
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " - " + y);
							}
							break;
						case 0x68: // imul
						case 0x69: // lmul
						case 0x6a: // fmul
						case 0x6b: // dmul
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " * " + y);
							}
							break;
						case 0x6c: // idiv
						case 0x6d: // ldiv
						case 0x6e: // fdiv
						case 0x6f: // ddiv
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " * " + y);
							}
							break;
						case 0x70: // irem
						case 0x71: // lrem
						case 0x72: // frem
						case 0x73: // drem
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " % " + y);
							}
							break;
						case 0x74: // ineg
						case 0x75: // lneg
						case 0x76: // fneg
						case 0x77: // dneg
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("-" + x);
							}
							break;
						case 0x78: // ishl
						case 0x79: // lshl
						case 0x7c: // iushr
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " << " + y);
							}
							break;
						case 0x7a: // ishr
						case 0x7b: // lshr
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " >> " + y);
							}
							break;
						case 0x7d: // lushr
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " >>> " + y);
							}
							break;
						case 0x7e: // iand
						case 0x7f: // iand
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " & " + y);
							}
							break;
						case 0x80: // ior
						case 0x81: // lor
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " | " + y);
							}
							break;
						case 0x82: // ixor
						case 0x83: // lxor
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " | " + y);
							}
							break;
						case 0x84: // iinc
							{
								int index = ref[++zz];
								int byte = ref[++zz];
								W("i");
								W(index);
								W(" += ");
								W(byte);
								W(";\n");
							}
							break;
						case 0x85: // i2l
						case 0x8c: // f2l
						case 0x8f: // d2l
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("(long)" + x);
							}
							break;
						case 0x86: // i2f
						case 0x89: // l2f
						case 0x90: // d2f
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("(float)" + x);
							}
							break;
						case 0x87: // i2d
						case 0x8a: // l2d
						case 0x8d: // f2d
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("(double)" + x);
							}
							break;
						case 0x88: // l2i
						case 0x8b: // f2i
						case 0x8e: // d2i
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("(int)" + x);
							}
							break;
						case 0x91: // i2b
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("(byte)" + x);
							}
							break;
						case 0x92: // i2c
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("(char)" + x);
							}
							break;
						case 0x93: // i2s
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("(short)" + x);
							}
							break;
						case 0x94: // lcmp
						case 0x95: // fcmpl
						case 0x96: // fcmpg
						case 0x97: // dcmpl
						case 0x98: // dcmpg
							{
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " - " + y);
							}
							break;
							// --
						case 0xa2: // if_icmpge
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2) + opcodePos;
								cout << "DEBUG if: " << ((b1 << 8) + b2) << ", " << opcodePos << " = " << idx << endl;
								
								std::string x = jvm_stack.back();
								jvm_stack.pop_back();
								std::string y = jvm_stack.back();
								jvm_stack.pop_back();
								W("if(" + x + " > " + y + ") {\n");
								
								// if goto before closing bracket
								unsigned char gotoTarget = ref[idx - 3 + 8];
								if(gotoTarget == 0xa7)
								{
									jumpTargets[idx].push_back("} else {\n");
									unsigned char b1 = ref[idx - 2 + 8];
									unsigned char b2 = ref[idx - 1 + 8];
									int idxGoto = ((b1 << 8) + b2) + idx - 3;
									jumpTargets[idxGoto].push_back("}\n");
								}
								else
								{
									jumpTargets[idx].push_back("}\n");
								}
							}
							break;
							// --
						case 0xa7: // goto
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2) + opcodePos;
								cout << "goto: " << ((b1 << 8) + b2) << ", " << opcodePos << ", " << idx << endl;
							}
							break;
							// --
						case 0xac: // ireturn
						case 0xad: // lreturn
						case 0xae: // freturn
						case 0xaf: // dreturn
						case 0xb0: // areturn
							W("return ");
							W(jvm_stack.back());
							W(";\n");
							jvm_stack.pop_back();
							break;
						case 0xb1: // return
							jvm_stack.clear();
							/*
							static { i = 3; } gets a "return" opcode,
							since "static { i = 3; return; } is invalid,
							we need to get rid of it
							*/
							if(!skip_return)
							{
								W("return;\n");
							}
							break;
						case 0xb2: // getstatic
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								std::vector<std::string> params = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								std::string retour = params.back();
								params.pop_back();
								
								std::string func_call = checkClassName(getName(class_index_info.ClassInfo.name_index)) + "." + getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								jvm_stack.push_back(func_call);
							}
							break;
						case 0xb3: // putstatic
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								std::vector<std::string> params = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								std::string retour = params.back();
								params.pop_back();
								
								std::string tmp = getName(class_index_info.ClassInfo.name_index) + "." + getName(name_and_type_index_info.NameAndTypeInfo.name_index) + " = " + jvm_stack[jvm_stack.size() - 1] + ";\n";
								
								W(tmp);
								
								jvm_stack.pop_back();
							}
							break;
						case 0xb4: // getfield
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								
								// CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								std::vector<std::string> params = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								std::string retour = params.back();
								params.pop_back();
								
								std::string tmp = jvm_stack.back() + "." + getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								
								jvm_stack.pop_back();
								jvm_stack.push_back(tmp);
							}
							break;
						case 0xb5: // putfield
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								
								// CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								std::vector<std::string> params = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								std::string retour = params.back();
								params.pop_back();
								
								std::string func_call = checkClassName(jvm_stack[jvm_stack.size() - 2]) + "." + getName(name_and_type_index_info.NameAndTypeInfo.name_index) + " = " + jvm_stack[jvm_stack.size() - 1] + ";\n";
								
								W(func_call);
								
								jvm_stack.pop_back();
								jvm_stack.pop_back();
							}
							break;
						case 0xb6: // invokevirtual
						case 0xb7: // invokespecial
							{
								bool invokevirtual = (c == 0xb6);
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								std::string cii_name = checkClassName(getName(class_index_info.ClassInfo.name_index));
								std::string fun_name = getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								
								std::vector<std::string> parametres = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								std::string returnType = parametres.back();
								parametres.pop_back(); // remove the return type
								
								std::string objectCalledUpon = jvm_stack[jvm_stack.size() - parametres.size() - 1];
								
								bool isNewCalled = false;
								bool isInit = false;
								
								if(fun_name == "<init>")
								{
									if(isCtor && objectCalledUpon == "this")
									{
										// change this = new ParentClass() to this.super()
										fun_name = "super";
									}
									else
									{
										isNewCalled = true;
										fun_name = cii_name;
									}
									
									isInit = true;
								}
								
								std::string fun_call;
								if(!isInit && !invokevirtual)
									fun_call += "((" + varTypes[objectCalledUpon] + ")";
								fun_call += objectCalledUpon;
								if(!isInit && !invokevirtual)
									fun_call += ")";
								
								if(isNewCalled)
								{
									fun_call += " = new ";
								}
								else
								{
									fun_call += ".";
								}
								
								fun_call += fun_name;
								fun_call += "(";
								for(std::size_t pp = 0;pp < parametres.size();pp++)
								{
									if(pp > 0)
										fun_call += ", ";
									
									fun_call += jvm_stack[jvm_stack.size() - parametres.size() + pp];
								}
								
								// <= to remove also the ObjectRef
								for(std::size_t i = 0;i <= parametres.size();i++)
								{
									jvm_stack.pop_back();
								}
								fun_call += ");\n";
								
								if(returnType != "void")
								{
									std::string retName = "ret" + returnType;
									std::string retNameAndOrType = retName;
									if(std::find(retNames.begin(), retNames.end(), returnType) == retNames.end())
									{
										varTypes[retName] = returnType;
										retNames.push_back(returnType);
										
										retNameAndOrType = returnType + " " + retName;
									}
									fun_call = retNameAndOrType + " = " + fun_call;
									jvm_stack.push_back(retName);
								}
								
								W(fun_call);
							}
							break;
						case 0xb8: // invokestatic
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								std::string cii_name = checkClassName(getName(class_index_info.ClassInfo.name_index));
								std::string fun_name = getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								
								std::vector<std::string> parametres = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								parametres.pop_back(); // remove the return type
								
								std::string fun_call = cii_name + "." + fun_name;
								fun_call += "(";
								for(std::size_t pp = 0;pp < parametres.size();pp++)
								{
									if(pp > 0)
										fun_call += ", ";
									
									fun_call += jvm_stack[jvm_stack.size() - parametres.size() + pp];
								}
								
								for(std::size_t i = 0;i < parametres.size();i++)
								{
									jvm_stack.pop_back();
								}
								fun_call += ");\n";
								
								W(fun_call);
							}
							break;
						case 0xb9: // invokeinterface
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								++zz; // int count = ref[++zz]; // unused
								++zz; // int zero = ref[++zz]; // unused
								
								CPinfo info = constant_pool[idx];
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								std::string cii_name = checkClassName(getName(class_index_info.ClassInfo.name_index));
								std::string fun_name = getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								
								std::vector<std::string> parametres = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								std::string returnType = parametres.back();
								parametres.pop_back(); // remove the return type
								
								std::string objectCalledUpon = jvm_stack[jvm_stack.size() - parametres.size() - 1];
								bool isNewCalled = false;
								
								if(fun_name == "<init>")
								{
									if(isCtor && objectCalledUpon == "this")
									{
										// change this = new ParentClass() to this.super()
										fun_name = "super";
									}
									else
									{
										isNewCalled = true;
										fun_name = cii_name;
									}
								}
								
								std::string fun_call;
								if(!isNewCalled)
									fun_call += "((" + varTypes[objectCalledUpon] + ")";
								fun_call += objectCalledUpon;
								if(!isNewCalled)
									fun_call += ")";
								
								if(isNewCalled)
								{
									fun_call += " = new ";
								}
								else
								{
									fun_call += ".";
								}
								
								fun_call += fun_name;
								fun_call += "(";
								for(std::size_t pp = 0;pp < parametres.size();pp++)
								{
									if(pp > 0)
										fun_call += ", ";
									
									fun_call += jvm_stack[jvm_stack.size() - parametres.size() + pp];
								}
								
								// <= to remove also the ObjectRef
								for(std::size_t i = 0;i <= parametres.size();i++)
								{
									jvm_stack.pop_back();
								}
								fun_call += ");\n";
								
								if(returnType != "void")
								{
									std::string retName = "ret" + returnType;
									std::string retNameAndOrType = retName;
									if(std::find(retNames.begin(), retNames.end(), returnType) == retNames.end())
									{
										varTypes[retName] = returnType;
										retNames.push_back(returnType);
										
										retNameAndOrType = returnType + " " + retName;
									}
									fun_call = retNameAndOrType + " = " + fun_call;
									jvm_stack.push_back(retName);
								}
								
								W(fun_call);
							}
							break;
							// --
						case 0xbb: // new
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								CPinfo class_name = constant_pool[info.ClassInfo.name_index];
								std::string className = checkClassName(class_name.UTF8Info.bytes);
								
								if(std::find(tmpNames.begin(), tmpNames.end(), className) == tmpNames.end())
								{
									W(className + " tmp" + className + ";\n");
									tmpNames.push_back(className);
								}
								
								jvm_stack.push_back("tmp" + className);
								varTypes["tmp" + className] = className;
							}
							break;
						case 0xbc: // newarray
							{
								int typeId = ref[++zz];
								std::string type = typeFromInt(typeId);
								std::string size = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("new " + type + "[" + size + "]");
								varTypes["new " + type + "[" + size + "]"] = type + "[]";
								// W(type + "[] newArr = new " + type + "[" + size + "];\n");
								// jvm_stack.push_back("newArr");
								// varTypes["newArr"] = type + "[]";
							}
							break;
						case 0xbd:
							{
								unsigned char b1 = ref[++zz];
								unsigned char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								CPinfo class_name = constant_pool[info.ClassInfo.name_index];
								std::string className = checkClassName(class_name.UTF8Info.bytes);
								
								std::string size = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back("new " + className + "[" + size + "]");
								varTypes["new " + className + "[" + size + "]"] = className + "[]";
							}
							break;
						case 0xbe: // arraylength
							{
								std::string arr = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(arr + ".length");
							}
							break;
							// --
						case 0xca: // breakpoint
							cerr << "reserved for breakpoints in Java debuggers; should not appear in any class file." << endl;
							break;
						/* 0xcb to 0xdf are reserved for future use */
						case 0xfe: // impdep1
						case 0xff: // impdep2
							cerr << "reserved for implementation-dependent operations within debuggers; should not appear in any class file." << endl;
							break;
						default:
							cerr << "Not managed opcode:" << std::hex << static_cast<int>(c) << endl;
							W("// Not managed opcode: ");
							W(std::hex << static_cast<int>(c));
							W("\n");
					}
					file.flush();
				}
				
				cerr << "JUMPTARGETS SIZE IS " << jumpTargets.size() << endl;
			}
			else
			{
				W("/*\n");
				W(std::get<0>(a));
				W("\n");
				W("***\n");
				W(std::get<1>(a));
				W("\n");
				W("*/\n");
			}
		}
		W("}\n\n");
	}
	
	for(FieldOutput f : output.fields)
	{
		if(f.isPublic)
			W("public ");
		if(f.isProtected)
			W("protected ");
		if(f.isPrivate)
			W("private ");
		if(f.isVolatile)
			W("volatile ");
		if(f.isTransient)
			W("transient ");
		if(f.isFinal)
			W("final ");
		if(f.isStatic)
			W("static ");
		
		W(f.type);
		W(" ");
		W(f.name);
		W(";\n");
	}
	
	W("}\n");
}

std::string ClassFile::checkClassName(std::string classname)
{
	std::replace(classname.begin(), classname.end(), '/', '.');
	size_t start_pos = classname.find("java.lang.");
	if(start_pos != std::string::npos)
	{
		classname.replace(start_pos, 10, std::string());
	}
	return classname;
}

char ClassFile::letterFromType(std::string type)
{
	if(type == "int")
		return 'i';
	else if(type == "long")
		return 'l';
	else if(type == "float")
		return 'f';
	else if(type == "double")
		return 'd';
	else
		return 'a';
}

std::string ClassFile::typeFromInt(int typeInt)
{
	std::string ret = "*ERROR*";
	switch(typeInt)
	{
		case T_BOOLEAN:
			ret = "boolean";
			break;
		case T_CHAR:
			ret = "char";
			break;
		case T_FLOAT:
			ret = "float";
			break;
		case T_DOUBLE:
			ret = "double";
			break;
		case T_BYTE:
			ret = "byte";
			break;
		case T_SHORT:
			ret = "short";
			break;
		case T_INT:
			ret = "int";
			break;
		case T_LONG:
			ret = "long";
			break;
		default:
			;
	}
	return ret;
}
