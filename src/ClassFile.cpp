/*
JDecomqiler

Copyright (c) <2011> <Alexander Roper>

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
#include <QFile>
#include <QDebug>

ClassFile::ClassFile(QString filename)
{
	init_init();
	
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
		return;
	
	stream.setDevice(&file);
	
	quint32 magic;
	stream >> magic;
	if(magic != 0xcafebabe)
	{
		qDebug().nospace() << "magic number is " << magic;
		return;
	}
	
	quint16 major, minor;
	stream >> minor >> major;
	
	quint16 constant_pool_count;
	stream >> constant_pool_count;
	qDebug().nospace() << constant_pool_count << " constants";
	
	constant_pool.push_back(CPinfo()); // index 0 is invalid
	for(int i = 1;i < constant_pool_count;i++)
	{
		if(parseConstant()) // return true if double or bigint
		{
			constant_pool.push_back(CPinfo()); // double and bigint use 2 indexes
			i++;
		}
	}
	
	quint16 access_flags, this_class, super_class;
	stream >> access_flags >> this_class >> super_class;
	
	if(access_flags & 0x0001)
		output.isPublic = true;
	if(access_flags & 0x0010)
		output.isFinal = true;
	if(access_flags & 0x0020)
		qDebug() << "is super";
	if(access_flags & 0x0200)
		output.isInterface = true;
	if(access_flags & 0x0400)
		output.isAbstract = true;
	if(access_flags & (~0x0631))
		qDebug() << "ERROR: unrecognized flag(s)";

	
	output.name = getName(constant_pool[this_class].ClassInfo.name_index);
	output.extends = checkClassName(getName(constant_pool[super_class].ClassInfo.name_index));
	
	quint16 interfaces_count;
	stream >> interfaces_count;
	qDebug().nospace() << interfaces_count << " interfaces";
	for(int i = 0;i < interfaces_count;i++)
	{
		parseInterface();
	}
	stream.skipRawData(interfaces_count * 2);
	
	quint16 fields_count;
	stream >> fields_count;
	qDebug().nospace() << fields_count << " fields";
	for(int i = 0;i < fields_count;i++)
	{
		output.fields.push_back(parseField());
	}
	
	quint16 methods_count;
	stream >> methods_count;
	qDebug().nospace() << methods_count << " methods";
	for(int i = 0;i < methods_count;i++)
	{
		output.methods.push_back(parseMethod());
	}
	
	quint16 attributes_count;
	stream >> attributes_count;
	qDebug().nospace() << attributes_count << " attributes";
	for(int i = 0;i < attributes_count;i++)
	{
		parseAttribute();
	}
}

ClassFile::~ClassFile()
{
	qDeleteAll(toDelete);
}

void ClassFile::init_init()
{
	for(int i = 0;i < 4;i++)
		istore[i] = false;
	
	skip_return = false;
}

#include <cstdio>

QPair<QString, QByteArray> ClassFile::parseAttribute()
{
	quint16 attribute_name_index;
	stream >> attribute_name_index;
	
	QString name = getName(attribute_name_index);
	
	quint32 length;
	stream >> length;
	char * tmp = new char[length];
	stream.readRawData(tmp, length);
	QByteArray data(tmp, length);
	delete tmp;
	
	return QPair<QString, QByteArray>(name, data);
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
				quint16 length;
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
			qFatal("ERROR: unrecognize TAG");
	}
	
	constant_pool.push_back(info);
	
	return isDoubleSize;
}

FieldOutput ClassFile::parseField()
{
	FieldOutput field;
	
	quint16 access_flags, name_index, descriptor_index;
	stream >> access_flags >> name_index >> descriptor_index;
	
	field.name = getName(name_index);
	int i = 0;
	field.type = parseType(getName(descriptor_index), i);
	
	if(access_flags & 0x0001)
		field.isPublic = true;
	if(access_flags & 0x0002)
		field.isPrivate = true;
	if(access_flags & 0x0004)
		field.isProtected = true;
	if(access_flags & 0x0008)
		field.isStatic = true;
	if(access_flags & 0x0010)
		field.isFinal = true;
	if(access_flags & 0x0040)
		field.isVolatile = true;
	if(access_flags & 0x0080)
		field.isTransient = true;
	if(access_flags & (~0x00DF))
		qDebug() << "ERROR: unrecognized flag(s)";
	
	quint16 attributes_count;
	stream >> attributes_count;
	qDebug().nospace() << "- " << attributes_count << " attributes";
	for(int i = 0;i < attributes_count;i++)
	{
		field.attributes.push_back(parseAttribute());
	}
	
	return field;
}

void ClassFile::parseInterface()
{
	stream.skipRawData(2);
}

MethodOutput ClassFile::parseMethod()
{
	MethodOutput method;
	
	quint16 access_flags, name_index, descriptor_index;
	stream >> access_flags >> name_index >> descriptor_index;
	method.name = getName(name_index);
	
	if(access_flags & 0x0001)
		method.isPublic = true;
	if(access_flags & 0x0002)
		method.isPrivate = true;
	if(access_flags & 0x0004)
		method.isProtected = true;
	if(access_flags & 0x0008)
		method.isStatic = true;
	if(access_flags & 0x0010)
		method.isFinal = true;
	if(access_flags & 0x0020)
		qDebug() << "is synchronized";
	if(access_flags & 0x0100)
		qDebug() << "is native";
	if(access_flags & 0x0400)
		method.isAbstract = true;
	if(access_flags & 0x0800)
		qDebug() << "is strictfp";
	if(access_flags & (~0x0D3F))
		qDebug() << "ERROR: unrecognized flag(s)";
	
	method.parametersType = parseSignature(getName(descriptor_index));
	method.returnType = method.parametersType.last();
	method.parametersType.pop_back();
	
	quint16 attributes_count;
	stream >> attributes_count;
	for(int i = 0;i < attributes_count;i++)
	{
		method.attributes.push_back(parseAttribute());
	}
	
	return method;
}

QVector<QString> ClassFile::parseSignature(QString signature)
{
	QVector<QString> params;
	
	int i = 0;
	if(signature[i] == '(')
	{
		i++;
		
		int next_is_array = 0;
		QString type;
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
			
			if(!type.isEmpty())
			{
				QString tmp = type;
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

QString ClassFile::parseType(QString signature, int & i)
{
	QString tmp;
	switch(signature[i].toAscii())
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
			tmp.chop(1);
			break;
		default:
			qDebug() << "ERROR:" << signature[i];
			qFatal("unrecognized parameter type in signature");
	}
	
	tmp = checkClassName(tmp);
	
	return tmp;
}

QString ClassFile::getName(quint16 index)
{
	QString ret_string("*ERROR*");
	
	CPinfo *data = &constant_pool[index];
	if(data->tag == CONSTANT_Utf8)
	{
		ret_string = QString(data->UTF8Info.bytes);
	}
	
	return ret_string;
}

#define W(c) file.write(c)
void ClassFile::generate()
{
	QFile file("output.java");
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
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
	
	W(output.name.toAscii());
	W(" extends ");
	W(output.extends.toAscii());
	W(" {\n");
	
	foreach(MethodOutput m, output.methods)
	{
		init_init();
		
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
			skip_return = true;
		}
		else
		{
			W(m.returnType.toAscii());
			W(" ");
			if(m.name == "<init>")
				W(output.name.toAscii());
			else
				W(m.name.toAscii());
			W("(");
			int pos = 1;
			foreach(QString str, m.parametersType)
			{
				if(pos > 1)
					W(", ");
				W(str.toAscii());
				W(" param");
				W(QByteArray::number(pos));
				pos++;
			}
			W(") ");
		}
		W("{\n");
		
		for(int i = 0;i < m.attributes.size();i++)
		{
			QPair<QString, QByteArray> a = m.attributes[i];
			
			if(a.first == "Code")
			{
				QByteArray & ref = a.second;
				int zz = 0;
				
				QVector<QString> jvm_stack;
				
				W("/*\n");
				
				W("stack size: ");
				char t1 = ref[zz++];
				char t2 = ref[zz++];
				int stack = ((t1 << 8) + t2);
				W(QByteArray::number(stack));
				
				W("\nhow many locals: ");
				t1 = ref[zz++];
				t2 = ref[zz++];
				int locals = ((t1 << 8) + t2);
				W(QByteArray::number(locals));
				
				W("\ncode size: ");
				t1 = ref[zz++];
				t2 = ref[zz++];
				char t3 = ref[zz++];
				char t4 = ref[zz++];
				int code_size = ((t1 << 24) + (t2 << 16) + (t3 << 8) + t4);
				W(QByteArray::number(code_size));
				W("\n");
				W("*/\n");
				int end = code_size + 8;
				for(;zz < end;zz++)
				{
					unsigned char c = ref[zz];
					switch(c)
					{
						case 0x00: // nop
							// skip
							break;
						case 0x01: // aconst_null
							W("aconst_null\n");
							break;
						case 0x02: // iconst_m1
							jvm_stack.push_back("-1");
							break;
						case 0x03: // iconst_0
							W("iconst_0\n");
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
							W("iconst_5\n");
							break;
						case 0x09: // lconst_0
							W("lconst_0\n");
							break;
						case 0x0a: // lconst_1
							W("lconst_1\n");
							break;
						case 0x12: // ldc
							{
								int idx = ref[++zz];
								switch(constant_pool[idx].tag)
								{
									case CONSTANT_String:
										jvm_stack.push_back("\""+getName(constant_pool[idx].StringInfo.string_index)+"\"");
										break;
									default:
										qDebug() << "tag :" << constant_pool[idx].tag;
										qFatal("0x12: unrecognized tag");
								}
							}
							break;
						case 0x1a: // iload_0
							if(m.parametersType.size() > 0)
								jvm_stack.push_back("param1");
							else
								jvm_stack.push_back("i1");
							break;
						case 0x1b: // iload_1
							if(m.parametersType.size() > 0)
								if(m.isStatic)
									jvm_stack.push_back("param2");
								else
									jvm_stack.push_back("param1");
							else
								jvm_stack.push_back("i1");
							break;
						case 0x2a: // aload_0
							jvm_stack.push_back("this");
							break;
						case 0x3c: // istore_1
							if(!istore[1])
							{
								W("int ");
								istore[1] = true;
							}
							W("i1 = ");
							W(jvm_stack.back().toAscii());
							W(";\n");
							jvm_stack.pop_back();
							break;
						case 0x3d: // istore_2
							if(!istore[2])
							{
								W("int ");
								istore[2] = true;
							}
							W("i2 = ");
							W(jvm_stack.back().toAscii());
							W(";\n");
							jvm_stack.pop_back();
							break;
						case 0x57: // pop
							W(jvm_stack.back().toAscii());
							W(";\n");
							jvm_stack.pop_back();
							break;
						case 0x60: // iadd
							{
								QString x = jvm_stack.back();
								jvm_stack.pop_back();
								QString y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " + " + y);
							}
							break;
						case 0x68: // imul
							{
								QString x = jvm_stack.back();
								jvm_stack.pop_back();
								QString y = jvm_stack.back();
								jvm_stack.pop_back();
								jvm_stack.push_back(x + " * " + y);
							}
							break;
						case 0xac: // ireturn
							{
								QString ret = jvm_stack.back();
								jvm_stack.clear();
								if(!skip_return)
								{
									W("return ");
									W(ret.toAscii());
									W(";\n");
								}
								skip_return = false;
							}
							break;
						case 0xb1: // return
							jvm_stack.clear();
							if(!skip_return)
							{
								W("return;\n");
							}
							skip_return = false;
							break;
						case 0xb2: // getstatic
							{
								char b1 = ref[++zz];
								char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								QVector<QString> params = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								QString retour = params.back();
								params.pop_back();
								
								QString func_call = checkClassName(getName(class_index_info.ClassInfo.name_index)) + "." + getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								jvm_stack.push_back(func_call);
							}
							break;
						case 0xb3: // putstatic
							{
								char b1 = ref[++zz];
								char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								QVector<QString> params = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								QString retour = params.back();
								params.pop_back();
								
								QString tmp = getName(class_index_info.ClassInfo.name_index) + "." + getName(name_and_type_index_info.NameAndTypeInfo.name_index) + " = " + jvm_stack[jvm_stack.size() - 1] + ";\n";
								qDebug() << "lol" << tmp;
								
								W(tmp.toAscii());
								
								jvm_stack.pop_back();
							}
							break;
						case 0xb4: // getfield
							{
								char b1 = ref[++zz];
								char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								QVector<QString> params = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								QString retour = params.back();
								params.pop_back();
								
								QString tmp = jvm_stack.back() + "." + getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								
								jvm_stack.pop_back();
								jvm_stack.push_back(tmp);
							}
							break;
						case 0xb5: // putfield
							{
								char b1 = ref[++zz];
								char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								QVector<QString> params = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								QString retour = params.back();
								params.pop_back();
								
								QString func_call = checkClassName(jvm_stack[jvm_stack.size() - 2]) + "." + getName(name_and_type_index_info.NameAndTypeInfo.name_index) + " = " + jvm_stack[jvm_stack.size() - 1] + ";\n";
								
								W(func_call.toAscii());
								
								jvm_stack.pop_back();
								jvm_stack.pop_back();
							}
							break;
						case 0xb6: // invokevirtual
							{
								char b1 = ref[++zz];
								char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								//CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								//qDebug() << "invoke virtual" << getName(class_index_info.ClassInfo.name_index);
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								QVector<QString> parametres = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								parametres.pop_back(); // remove the return type
								
								W(jvm_stack[jvm_stack.size() - parametres.size() - 1].toAscii());
								W(".");
								W(getName(name_and_type_index_info.NameAndTypeInfo.name_index).toAscii());
								W("(");
								for(int pp = 0;pp < parametres.size();pp++)
								{
									if(pp > 0)
										W(", ");
									
									W(jvm_stack[jvm_stack.size() - parametres.size() + pp].toAscii());
								}
								jvm_stack.remove(jvm_stack.size() - parametres.size() - 1, parametres.size() + 1);
								W(");\n");
							}
							break;
						case 0xb7: // invokespecial
							{
								char b1 = ref[++zz];
								char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								QString cii_name = checkClassName(getName(class_index_info.ClassInfo.name_index));
								QString fun_name = getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								if(fun_name == "<init>")
									fun_name = "super";
								
								QVector<QString> parametres = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								parametres.pop_back(); // remove the return type
								
								QString fun_call = jvm_stack[jvm_stack.size() - parametres.size() - 1];
								fun_call += ".";
								fun_call += fun_name;
								fun_call += "(";
								for(int pp = 0;pp < parametres.size();pp++)
								{
									if(pp > 0)
										fun_call += ", ";
									
									fun_call += jvm_stack[jvm_stack.size() - parametres.size() + pp];
								}
								jvm_stack.remove(jvm_stack.size() - parametres.size() - 1, parametres.size() + 1);
								fun_call += ")";
								
								jvm_stack.push_back(fun_call);
							}
							break;
						case 0xb8: // invokestatic
							{
								char b1 = ref[++zz];
								char b2 = ref[++zz];
								int idx = ((b1 << 8) + b2);
								
								CPinfo info = constant_pool[idx];
								CPinfo class_index_info = constant_pool[info.RefInfo.class_index];
								CPinfo name_and_type_index_info = constant_pool[info.RefInfo.name_and_type_index];
								
								QString cii_name = checkClassName(getName(class_index_info.ClassInfo.name_index));
								QString fun_name = getName(name_and_type_index_info.NameAndTypeInfo.name_index);
								
								QVector<QString> parametres = parseSignature(getName(name_and_type_index_info.NameAndTypeInfo.descriptor_index));
								parametres.pop_back(); // remove the return type
								
								QString fun_call = cii_name + "." + fun_name;
								fun_call += "(";
								for(int pp = 0;pp < parametres.size();pp++)
								{
									if(pp > 0)
										fun_call += ", ";
									
									fun_call += jvm_stack[jvm_stack.size() - parametres.size() + pp];
								}
								jvm_stack.remove(jvm_stack.size() - parametres.size(), parametres.size());
								fun_call += ")";
								
								jvm_stack.push_back(fun_call);
							}
							break;
						/*case 0x:
							W("");
							break;
						case 0x:
							W("");
							break;
						case 0x:
							W("");
							break;*/
						default:
							W(QByteArray::number(c));
							W("\n");
					}
					file.flush();
				}
			}
			else
			{
				W("/*\n");
				W(a.first.toAscii());
				W("\n");
				W("***\n");
				W(a.second.toHex());
				W("\n");
				W("*/\n");
			}
		}
		W("}\n\n");
	}
	
	foreach(FieldOutput f, output.fields)
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
		
		W(f.type.toAscii());
		W(" ");
		W(f.name.toAscii());
		W(";\n");
	}
	
	W("}\n");
}

QString ClassFile::checkClassName(QString classname)
{
	classname.replace('/', '.');
	classname.remove("java.lang.");
	return classname;
}
