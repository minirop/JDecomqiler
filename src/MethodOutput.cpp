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
#include "MethodOutput.h"
#include "Helpers.h"
#include "opcodes.h"
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>

using namespace std;

#define W(c) file << c
#define BUFF(c) bufferMethod.push_back(c);

#define STORE(type, value, index) \
	{ \
		std::string buffOutput; \
		if(!store[index]) \
		{ \
			buffOutput += type; \
			buffOutput += " "; \
			store[index] = true; \
		} \
		buffOutput += letterFromType(type); \
		buffOutput += std::to_string(index); \
		buffOutput += " = "; \
		buffOutput += value; \
		buffOutput += ";\n"; \
		jvm_stack.pop_back(); \
		BUFF(buffOutput); \
	}

#define STORE_OBJECT(type, value, index) \
	{ \
		std::string buffOutput; \
		if(objectVariables[index].first != type) \
		{ \
			if(!objectTypeCounter.count(type)) \
			{ \
				objectTypeCounter[type] = 0; \
			} \
			\
			buffOutput += type; \
			buffOutput += " "; \
			objectVariables[index].first = type; \
			objectVariables[index].second = removeArray(type) + std::to_string(objectTypeCounter[type]); \
			varTypes[objectVariables[index].second] = type; \
			objectTypeCounter[type]++; \
		} \
		\
		buffOutput += objectVariables[index].second; \
		buffOutput += " = "; \
		buffOutput += value; \
		buffOutput += ";\n"; \
		jvm_stack.pop_back(); \
		BUFF(buffOutput); \
	}

#define IF_OPCODE(op) \
	{ \
		unsigned char b1 = ref[++zz]; \
		unsigned char b2 = ref[++zz]; \
		int idx = static_cast<signed short>((b1 << 8) + b2) + opcodePos; \
		\
		std::string value = jvm_stack.back(); \
		jvm_stack.pop_back(); \
		\
		if(idx > opcodePos) \
		{ \
			BUFF("if(" + value + " " op ") {\n"); \
			jumpTargets[idx].push_back("}\n"); \
		} \
		else \
		{ \
			BUFF("} while(" + value + " " op ");\n"); \
			jumpTargets[idx].push_back("do {\n"); \
		} \
	}

#define IF_OR_LOOP_OPCODE(op) \
	{ \
		unsigned char b1 = ref[++zz]; \
		unsigned char b2 = ref[++zz]; \
		int idx = static_cast<signed short>((b1 << 8) + b2) + opcodePos; \
		\
		std::string x = jvm_stack.back(); \
		jvm_stack.pop_back(); \
		std::string y = jvm_stack.back(); \
		jvm_stack.pop_back(); \
		\
		bool hasGoto = false; \
		int idxGoto = 0; \
		unsigned char gotoTarget = ref[idx - 3 + 8]; \
		if(gotoTarget == 0xa7) \
		{ \
			hasGoto = true; \
			unsigned char b1 = ref[idx - 2 + 8]; \
			unsigned char b2 = ref[idx - 1 + 8]; \
			idxGoto = static_cast<signed short>((b1 << 8) + b2) + idx - 3; \
		} \
		\
		if(hasGoto) \
		{ \
			if(idxGoto < opcodePos)\
			{ \
				BUFF("while(" + y + " " op " " + x + ") {\n"); \
				jumpTargets[idx].push_back("}\n"); \
			} \
			else \
			{ \
				BUFF("if(" + y + " " op " " + x + ") {\n"); \
				jumpTargets[idx].push_back("} else {\n"); \
				jumpTargets[idxGoto].push_back("}\n"); \
			} \
		} \
		else \
		{ \
			jumpTargets[idx].push_back("}\n"); \
		} \
	}

void MethodOutput::generate(std::ofstream & file)
{
	bool isCtor = false;
	
	if(isPublic)
		W("public ");
	if(isProtected)
		W("protected ");
	if(isPrivate)
		W("private ");
	if(isAbstract)
		W("abstract ");
	if(isStatic)
		W("static ");
	if(isFinal)
		W("final ");
	
	if(name == "<clinit>")
	{
	}
	else
	{
		if(name == "<init>")
		{
			W(thisClass);
			isCtor = true;
		}
		else
		{
			W(returnType);
			W(" ");
			W(name);
			
		}
		W("(");
		int pos = (isStatic ? 0 : 1);
		for(std::string str : parametersType)
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
	
	for(std::size_t i = 0;i < attributes.size();i++)
	{
		std::tuple<std::string, std::string> a = attributes[i];
		
		if(std::get<0>(a) == "Code")
		{
			const std::string & ref = std::get<1>(a);
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
			std::size_t paramCount = parametersType.size();
			for(std::size_t storei = 0;storei < paramCount;storei++)
			{
				store[storei] = true;
			}
			
			std::vector<std::string> retNames;
			std::vector<std::string> tmpNames;
			std::map<std::string, std::string> varTypes;
			std::map<int, std::vector<std::string>> jumpTargets;
			std::vector<std::string> bufferMethod;
			std::map<int, std::pair<std::string, std::string>> objectVariables; // int = variable ID, string = type of the object, string = name of the variable
			std::map<std::string, int> objectTypeCounter;
			bool nextInvokeIsNew = false;
			
			int end = code_size + 8;
			for(int opcodePos = 0;zz < end;zz++)
			{
				opcodePos = zz - 8;
				bufferMethod.push_back("/*" + std::to_string(opcodePos) + "*/ ");

				bool isLastOpcode = zz + 1 >= end;
				
				unsigned char c = ref[zz];
				switch(c)
				{
					case OP_nop:
						// skip
						break;
					case OP_aconst_null:
						jvm_stack.push_back("null");
						break;
					case OP_iconst_m1:
						jvm_stack.push_back("-1");
						break;
					case OP_iconst_0:
						jvm_stack.push_back("0");
						break;
					case OP_iconst_1:
						jvm_stack.push_back("1");
						break;
					case OP_iconst_2:
						jvm_stack.push_back("2");
						break;
					case OP_iconst_3:
						jvm_stack.push_back("3");
						break;
					case OP_iconst_4:
						jvm_stack.push_back("4");
						break;
					case OP_iconst_5:
						jvm_stack.push_back("5");
						break;
					case OP_lconst_0:
						jvm_stack.push_back("0L");
						break;
					case OP_lconst_1:
						jvm_stack.push_back("1L");
						break;
					case OP_fconst_0:
						jvm_stack.push_back("0.0f");
						break;
					case OP_fconst_1:
						jvm_stack.push_back("1.0f");
						break;
					case OP_fconst_2:
						jvm_stack.push_back("2.0f");
						break;
					case OP_dconst_0:
						jvm_stack.push_back("0.0");
						break;
					case OP_dconst_1:
						jvm_stack.push_back("1.0");
						break;
					case OP_bipush:
						{
							int idx = ref[++zz];
							jvm_stack.push_back(std::to_string(idx));
						}
						break;
					case OP_sipush:
						{
							unsigned char b1 = ref[++zz];
							unsigned char b2 = ref[++zz];
							int idx = ((b1 << 8) + b2);
							jvm_stack.push_back(std::to_string(idx));
						}
						break;
					case OP_ldc:
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
					case OP_ldc_w:
					case OP_ldc2_w:
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
					case OP_iload:
						{
							int idx = ref[++zz];
							jvm_stack.push_back(std::string("i") + std::to_string(idx));
						}
						break;
					case OP_lload:
						{
							int idx = ref[++zz];
							jvm_stack.push_back(std::string("l") + std::to_string(idx));
						}
						break;
					case OP_fload:
						{
							int idx = ref[++zz];
							jvm_stack.push_back(std::string("f") + std::to_string(idx));
						}
						break;
					case OP_dload:
						{
							int idx = ref[++zz];
							jvm_stack.push_back(std::string("d") + std::to_string(idx));
						}
						break;
					case OP_aload:
						{
							int idx = ref[++zz];
							jvm_stack.push_back(objectVariables[idx].second);
						}
						break;
					case OP_iload_0:
						jvm_stack.push_back("i0");
						break;
					case OP_iload_1:
						jvm_stack.push_back("i1");
						break;
					case OP_iload_2:
						jvm_stack.push_back("i2");
						break;
					case OP_iload_3:
						jvm_stack.push_back("i3");
						break;
					case OP_lload_0:
						jvm_stack.push_back("l0");
						break;
					case OP_lload_1:
						jvm_stack.push_back("l1");
						break;
					case OP_lload_2:
						jvm_stack.push_back("l2");
						break;
					case OP_lload_3:
						jvm_stack.push_back("l3");
						break;
					case OP_fload_0:
						jvm_stack.push_back("f0");
						break;
					case OP_fload_1:
						jvm_stack.push_back("f1");
						break;
					case OP_fload_2:
						jvm_stack.push_back("f2");
						break;
					case OP_fload_3:
						jvm_stack.push_back("f3");
						break;
					case OP_dload_0:
						jvm_stack.push_back("d0");
						break;
					case OP_dload_1:
						jvm_stack.push_back("d1");
						break;
					case OP_dload_2:
						jvm_stack.push_back("d2");
						break;
					case OP_dload_3:
						jvm_stack.push_back("d3");
						break;
					case OP_aload_0:
						if(isStatic)
							jvm_stack.push_back(objectVariables[0].second);
						else
							jvm_stack.push_back("this");
						break;
					case OP_aload_1:
						jvm_stack.push_back(objectVariables[1].second);
						break;
					case OP_aload_2:
						jvm_stack.push_back(objectVariables[2].second);
						break;
					case OP_aload_3:
						jvm_stack.push_back(objectVariables[3].second);
						break;
					case OP_iaload:
					case OP_laload:
					case OP_faload:
					case OP_daload:
					case OP_aaload:
					case OP_baload:
					case OP_caload:
					case OP_saload:
						{
							std::string index = jvm_stack.back();
							jvm_stack.pop_back();
							std::string arr = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(arr + "[" + index + "]");
							std::string oneDimensionLess = varTypes[arr];
							size_t start_pos = oneDimensionLess.find("[]");
							if(start_pos != std::string::npos)
							{
								oneDimensionLess.replace(start_pos, 10, std::string());
							}
							varTypes[arr + "[" + index + "]"] = oneDimensionLess;
							
						}
						break;
					case OP_istore:
						{
							int index = ref[++zz];
							STORE("int", jvm_stack.back(), index)
						}
						break;
					case OP_lstore:
						{
							int index = ref[++zz];
							STORE("long", jvm_stack.back(), index)
						}
						break;
					case OP_fstore:
						{
							int index = ref[++zz];
							STORE("float", jvm_stack.back(), index)
						}
						break;
					case OP_dstore:
						{
							int index = ref[++zz];
							STORE("double", jvm_stack.back(), index)
						}
						break;
					case OP_astore:
						{
							int index = ref[++zz];
							std::string varName = jvm_stack.back();
							STORE_OBJECT(varTypes[varName], varName, index)
						}
						break;
					case OP_istore_0:
						STORE("int", jvm_stack.back(), 0)
						break;
					case OP_istore_1:
						STORE("int", jvm_stack.back(), 1)
						break;
					case OP_istore_2:
						STORE("int", jvm_stack.back(), 2)
						break;
					case OP_istore_3:
						STORE("int", jvm_stack.back(), 3)
						break;
					case OP_lstore_0:
						STORE("long", jvm_stack.back(), 0)
						break;
					case OP_lstore_1:
						STORE("long", jvm_stack.back(), 1)
						break;
					case OP_lstore_2:
						STORE("long", jvm_stack.back(), 2)
						break;
					case OP_lstore_3:
						STORE("long", jvm_stack.back(), 3)
						break;
					case OP_fstore_0:
						STORE("float", jvm_stack.back(), 0)
						break;
					case OP_fstore_1:
						STORE("float", jvm_stack.back(), 1)
						break;
					case OP_fstore_2:
						STORE("float", jvm_stack.back(), 2)
						break;
					case OP_fstore_3:
						STORE("float", jvm_stack.back(), 3)
						break;
					case OP_dstore_0:
						STORE("double", jvm_stack.back(), 0)
						break;
					case OP_dstore_1:
						STORE("double", jvm_stack.back(), 1)
						break;
					case OP_dstore_2:
						STORE("double", jvm_stack.back(), 2)
						break;
					case OP_dstore_3:
						STORE("double", jvm_stack.back(), 3)
						break;
					case OP_astore_0:
						{
							std::string varName = jvm_stack.back();
							STORE_OBJECT(varTypes[varName], varName, 0)
						}
						break;
					case OP_astore_1:
						{
							std::string varName = jvm_stack.back();
							STORE_OBJECT(varTypes[varName], varName, 1)
						}
						break;
					case OP_astore_2:
						{
							std::string varName = jvm_stack.back();
							STORE_OBJECT(varTypes[varName], varName, 2)
						}
						break;
					case OP_astore_3:
						{
							std::string varName = jvm_stack.back();
							STORE_OBJECT(varTypes[varName], varName, 3)
						}
						break;
					case OP_iastore:
					case OP_lastore:
					case OP_fastore:
					case OP_dastore:
					case OP_aastore:
					case OP_bastore:
					case OP_castore:
					case OP_sastore:
						{
							std::string value = jvm_stack.back();
							jvm_stack.pop_back();
							std::string index = jvm_stack.back();
							jvm_stack.pop_back();
							std::string arr = jvm_stack.back();
							jvm_stack.pop_back();
							BUFF(arr + "[" + index + "] = " + value + ";\n");
						}
						break;
					case OP_pop:
					case OP_pop2: // since we don't treat double as 2 values
						jvm_stack.pop_back();
						break;
					case OP_dup:
						if(!nextInvokeIsNew)
							jvm_stack.push_back(jvm_stack.back());
						break;
					case OP_dup_x1:
						{
							std::string value1 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value2 = jvm_stack.back();
							jvm_stack.pop_back();
							
							jvm_stack.push_back(value1);
							jvm_stack.push_back(value2);
							jvm_stack.push_back(value1);
						}
						break;
					case OP_dup_x2:
						{
							std::string value1 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value2 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value3 = jvm_stack.back();
							jvm_stack.pop_back();
							
							jvm_stack.push_back(value1);
							jvm_stack.push_back(value3);
							jvm_stack.push_back(value2);
							jvm_stack.push_back(value1);
						}
						break;
					case OP_dup2:
						{
							std::string value1 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value2 = jvm_stack.back();
							jvm_stack.pop_back();
							
							jvm_stack.push_back(value2);
							jvm_stack.push_back(value1);
							jvm_stack.push_back(value2);
							jvm_stack.push_back(value1);
						}
						break;
					case OP_dup2_x1:
						{
							std::string value1 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value2 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value3 = jvm_stack.back();
							jvm_stack.pop_back();
							
							jvm_stack.push_back(value2);
							jvm_stack.push_back(value1);
							jvm_stack.push_back(value3);
							jvm_stack.push_back(value2);
							jvm_stack.push_back(value1);
						}
						break;
					case OP_dup2_x2:
						{
							std::string value1 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value2 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value3 = jvm_stack.back();
							jvm_stack.pop_back();
							std::string value4 = jvm_stack.back();
							jvm_stack.pop_back();
							
							jvm_stack.push_back(value2);
							jvm_stack.push_back(value1);
							jvm_stack.push_back(value4);
							jvm_stack.push_back(value3);
							jvm_stack.push_back(value2);
							jvm_stack.push_back(value1);
						}
						break;
					case OP_swap:
						{
							std::string first = jvm_stack.back();
							jvm_stack.pop_back();
							std::string second = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(first);
							jvm_stack.push_back(second);
						}
						break;
					case OP_iadd:
					case OP_ladd:
					case OP_fadd:
					case OP_dadd:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " + " + y);
						}
						break;
					case OP_isub:
					case OP_lsub:
					case OP_fsub:
					case OP_dsub:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " - " + y);
						}
						break;
					case OP_imul:
					case OP_lmul:
					case OP_fmul:
					case OP_dmul:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " * " + y);
						}
						break;
					case OP_idiv:
					case OP_ldiv:
					case OP_fdiv:
					case OP_ddiv:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " * " + y);
						}
						break;
					case OP_irem:
					case OP_lrem:
					case OP_frem:
					case OP_drem:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " % " + y);
						}
						break;
					case OP_ineg:
					case OP_lneg:
					case OP_fneg:
					case OP_dneg:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("-" + x);
						}
						break;
					case OP_ishl:
					case OP_lshl:
					case OP_iushr:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " << " + y);
						}
						break;
					case OP_ishr:
					case OP_lshr:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " >> " + y);
						}
						break;
					case OP_lushr:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " >>> " + y);
						}
						break;
					case OP_iand:
					case OP_land:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " & " + y);
						}
						break;
					case OP_ior:
					case OP_lor:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " | " + y);
						}
						break;
					case OP_ixor:
					case OP_lxor:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " | " + y);
						}
						break;
					case OP_iinc:
						{
							int index = ref[++zz];
							int byte = ref[++zz];
							if(byte < 0)
							{
								if(byte == -1)
								{
									BUFF("i" + std::to_string(index) + "--;\n");
								}
								else
								{
									BUFF("i" + std::to_string(index) + " -= " + std::to_string(byte) + ";\n");
								}
							}
							else
							{
								if(byte == 1)
								{
									BUFF("i" + std::to_string(index) + "++;\n");
								}
								else
								{
									BUFF("i" + std::to_string(index) + " += " + std::to_string(byte) + ";\n");
								}
							}
						}
						break;
					case OP_i2l:
					case OP_f2l:
					case OP_d2l:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("(long)" + x);
						}
						break;
					case OP_i2f:
					case OP_l2f:
					case OP_d2f:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("(float)" + x);
						}
						break;
					case OP_i2d:
					case OP_l2d:
					case OP_f2d:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("(double)" + x);
						}
						break;
					case OP_l2i:
					case OP_f2i:
					case OP_d2i:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("(int)" + x);
						}
						break;
					case OP_i2b:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("(byte)" + x);
						}
						break;
					case OP_i2c:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("(char)" + x);
						}
						break;
					case OP_i2s:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("(short)" + x);
						}
						break;
					case OP_lcmp:
					case OP_fcmpl:
					case OP_fcmpg:
					case OP_dcmpl:
					case OP_dcmpg:
						{
							std::string x = jvm_stack.back();
							jvm_stack.pop_back();
							std::string y = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(x + " - " + y);
						}
						break;
					case OP_ifeq:
						IF_OPCODE("!= 0")
						break;
					case OP_ifne:
						IF_OPCODE("== 0")
						break;
					case OP_iflt:
						IF_OPCODE(">= 0")
						break;
					case OP_ifge:
						IF_OPCODE("< 0")
						break;
					case OP_ifgt:
						IF_OPCODE("<= 0")
						break;
					case OP_ifle:
						IF_OPCODE("> 0")
						break;
					case OP_if_icmpeq:
						IF_OR_LOOP_OPCODE("!=")
						break;
					case OP_if_icmpne:
						IF_OR_LOOP_OPCODE("==")
						break;
					case OP_if_icmplt:
						IF_OR_LOOP_OPCODE(">=")
						break;
					case OP_if_icmpge:
						IF_OR_LOOP_OPCODE("<")
						break;
					case OP_if_icmpgt:
						IF_OR_LOOP_OPCODE("<=")
						break;
					case OP_if_icmple:
						IF_OR_LOOP_OPCODE(">")
						break;
					case OP_if_acmpeq:
						IF_OR_LOOP_OPCODE("!=")
						break;
					case OP_if_acmpne:
						IF_OR_LOOP_OPCODE("==")
						break;
					case OP_goto:
						// goto is not used directly, only checked in conditional opcodes to detect if an "if" is a loop or has an "else"
						zz += 2;
						break;
					case OP_jsr:
						{
							unsigned char b1 = ref[++zz];
							unsigned char b2 = ref[++zz];
							int idx = static_cast<signed short>((b1 << 8) + b2);
							
							BUFF("// jsr jump to: " + std::to_string(idx) + "\n");
							jvm_stack.push_back("/* ret addr: " + std::to_string(opcodePos) + " */");
						}
						break;
					case OP_ret:
						{
							unsigned char i = ref[++zz];
							cout << "RET to addr of local addr " << i << endl;
						}
						break;
					case OP_tableswitch:
						{
							// TODO
							cerr << "tableswitch not implemented, segfault incoming." << endl;
							int tableSwitchOpcodePosition = zz;
							zz += ((zz+1) % 4); // padding
							
							// default jump
							unsigned char d1 = ref[++zz];
							unsigned char d2 = ref[++zz];
							unsigned char d3 = ref[++zz];
							unsigned char d4 = ref[++zz];
							int defaultJump = static_cast<int>((d1 << 24) + (d2 << 16) + (d3 << 8) + d4);
							
							// low jump
							unsigned char l1 = ref[++zz];
							unsigned char l2 = ref[++zz];
							unsigned char l3 = ref[++zz];
							unsigned char l4 = ref[++zz];
							int lowJump = static_cast<int>((l1 << 24) + (l2 << 16) + (l3 << 8) + l4);
							
							// high jump
							unsigned char h1 = ref[++zz];
							unsigned char h2 = ref[++zz];
							unsigned char h3 = ref[++zz];
							unsigned char h4 = ref[++zz];
							int highJump = static_cast<int>((h1 << 24) + (h2 << 16) + (h3 << 8) + h4);
							
							// dunno where the 7 comes from.
							cout << "tableswitch: " << jvm_stack.back() << " => " << (defaultJump + tableSwitchOpcodePosition + 7) << ", " << lowJump << ", " << highJump << endl;
							
							for(int i = lowJump;i <= highJump;i++)
							{
								unsigned char jumpTarget1 = ref[++zz];
								unsigned char jumpTarget2 = ref[++zz];
								unsigned char jumpTarget3 = ref[++zz];
								unsigned char jumpTarget4 = ref[++zz];
								int jumpTarget = static_cast<int>((jumpTarget1 << 24) + (jumpTarget2 << 16) + (jumpTarget3 << 8) + jumpTarget4);
								cout << "jumpTarget " << i << " : " << (jumpTarget + tableSwitchOpcodePosition + 7) << endl;
							}
						}
						break;
					case OP_lookupswitch:
						// TODO
						cerr << "lookupswitch not implemented, segfault incoming." << endl;
						break;
					case OP_ireturn:
					case OP_lreturn:
					case OP_freturn:
					case OP_dreturn:
					case OP_areturn:
						BUFF("return " + jvm_stack.back() + ";\n");
						jvm_stack.pop_back();
						break;
					case OP_return:
						{
							if (!isLastOpcode)
							{
								BUFF("return;\n");
							}
							else
							{
								jvm_stack.clear();
							}
							break;
						}
					case OP_getstatic:
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
							
							std::string static_call;
							std::string staticClassName = checkClassName(getName(class_index_info.ClassInfo.name_index));
							if(staticClassName != thisClass)
							{
								static_call += staticClassName + ".";
							}
							static_call += getName(name_and_type_index_info.NameAndTypeInfo.name_index);
							jvm_stack.push_back(static_call);
						}
						break;
					case OP_putstatic:
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
							
							std::string staticClassName = getName(class_index_info.ClassInfo.name_index);
							std::string tmp;
							if(staticClassName != thisClass)
							{
								tmp += staticClassName + ".";
							}
							tmp += getName(name_and_type_index_info.NameAndTypeInfo.name_index) + " = " + jvm_stack[jvm_stack.size() - 1] + ";\n";
							
							BUFF(tmp);
							
							jvm_stack.pop_back();
						}
						break;
					case OP_getfield:
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
					case OP_putfield:
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
							
							BUFF(func_call);
							
							jvm_stack.pop_back();
							jvm_stack.pop_back();
						}
						break;
					case OP_invokevirtual:
					case OP_invokespecial:
						{
							// bool invokevirtual = (c == 0xb6);
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
							
							std::string fun_call;
							if(nextInvokeIsNew)
							{
								unsigned char nextOP = ref[zz+1];
								int next = nextOP;
								if(next != OP_pop)
								{
									if(!objectTypeCounter.count(cii_name))
									{
										objectTypeCounter[cii_name] = 0;
									}
									
									fun_call += cii_name;
									fun_call += " ";
									fun_call += removeArray(cii_name) + std::to_string(objectTypeCounter[cii_name]);
									objectTypeCounter[cii_name]++;
									fun_call += " = ";
									
									switch(next)
									{
										case OP_astore:
											zz++; // remove "index"
										case OP_astore_0:
										case OP_astore_1:
										case OP_astore_2:
										case OP_astore_3:
											zz++; // remove the opcode
										default:
											;
									}
								}
								else
								{
									jvm_stack.push_back("/* garbage */");
								}
								fun_call += "new " + cii_name;
							}
							else
							{
								if(fun_name == "<init>")
								{
									if(cii_name == parentClass)
										fun_name = "super";
									else
										fun_name = cii_name;
								}
								
								if(jvm_stack[jvm_stack.size() - parametres.size() - 1] != "this")
								{
									fun_call += jvm_stack[jvm_stack.size() - parametres.size() - 1] + ".";
								}
								else
								{
									if(fun_name == "super" && parentClass == "Object")
										break;
								}
								
								fun_call += fun_name;
							}
							
							fun_call += "(";
							for(std::size_t pp = 0;pp < parametres.size();pp++)
							{
								if(pp > 0)
									fun_call += ", ";
								
								fun_call += jvm_stack[jvm_stack.size() - parametres.size() + pp];
							}
							
							// remove the ObjectRef
							if(!nextInvokeIsNew)
								jvm_stack.pop_back();
							for(std::size_t i = 0;i < parametres.size();i++)
							{
								jvm_stack.pop_back();
							}
							fun_call += ")";
							
							if(nextInvokeIsNew)
							{
								BUFF(fun_call + ";\n");
							}
							else
							{
								if(returnType != "void")
								{
									jvm_stack.push_back(fun_call);
									/* probaly need to have this kind of code
									unsigned char nextOP = ref[zz+1];
									int next = nextOP;
									if(next == OP_pop)
										BUFF
									else
										jvm_stack.push_back
									*/
								}
								else
								{
									BUFF(fun_call + ";\n");
								}
							}
						}
						break;
					case OP_invokestatic:
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
							
							BUFF(fun_call);
						}
						break;
					case OP_invokeinterface:
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
							fun_call += ")";
							
							if(returnType != "void")
							{
								// std::string retName = "ret" + returnType;
								// std::string retNameAndOrType = retName;
								// if(std::find(retNames.begin(), retNames.end(), returnType) == retNames.end())
								// {
									// varTypes[retName] = returnType;
									// retNames.push_back(returnType);
									
									// retNameAndOrType = returnType + " " + retName;
								// }
								// fun_call = retNameAndOrType + " = " + fun_call;
								// jvm_stack.push_back(retName);
								jvm_stack.push_back(fun_call);
							}
							else
							{
								BUFF(fun_call + ";\n");
							}
						}
						break;
					case OP_invokedynamic: // (check if can be a new)
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
							if(!isInit)
								fun_call += "((" + varTypes[objectCalledUpon] + ")";
							fun_call += objectCalledUpon;
							if(!isInit)
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
							fun_call += ")";
							
							if(returnType != "void")
							{
								// std::string retName = "ret" + returnType;
								// std::string retNameAndOrType = retName;
								// if(std::find(retNames.begin(), retNames.end(), returnType) == retNames.end())
								// {
									// varTypes[retName] = returnType;
									// retNames.push_back(returnType);
									
									// retNameAndOrType = returnType + " " + retName;
								// }
								// fun_call = retNameAndOrType + " = " + fun_call;
								// jvm_stack.push_back(retName);
								jvm_stack.push_back(fun_call);
							}
							else
							{
								BUFF(fun_call + ";\n");
							}
						}
						break;
					case OP_new:
						{
							unsigned char b1 = ref[++zz];
							unsigned char b2 = ref[++zz];
							int idx = ((b1 << 8) + b2);
							
							if(ref[zz+1] != OP_dup || (ref[zz+2] & 0xff) != OP_invokespecial)
							{
								cerr << "ERROR: after new it's not dup followed by invokespecial" << endl;
							}
							
							nextInvokeIsNew = true;
							
							CPinfo info = constant_pool[idx];
							CPinfo class_name = constant_pool[info.ClassInfo.name_index];
							std::string className = checkClassName(class_name.UTF8Info.bytes);
							
							// jvm_stack.push_back(className);
							// if(std::find(tmpNames.begin(), tmpNames.end(), className) == tmpNames.end())
							// {
								// BUFF(className + " tmp" + className + ";\n");
								// tmpNames.push_back(className);
							// }
							
							// jvm_stack.push_back("tmp" + className);
							// varTypes["tmp" + className] = className;
						}
						break;
					case OP_newarray:
						{
							int typeId = ref[++zz];
							std::string type = typeFromInt(typeId);
							std::string size = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back("new " + type + "[" + size + "]");
							varTypes["new " + type + "[" + size + "]"] = type + "[]";
						}
						break;
					case OP_anewarray:
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
					case OP_arraylength:
						{
							std::string arr = jvm_stack.back();
							jvm_stack.pop_back();
							jvm_stack.push_back(arr + ".length");
						}
						break;
					case OP_athrow:
						{
							// TODO
							cerr << "athrow not implemented:" << endl;
							std::string exception = jvm_stack.back();
							jvm_stack.clear();
							jvm_stack.push_back(exception);
						}
						break;
					case OP_checkcast:
						{
							// TODO
							unsigned char b1 = ref[++zz];
							unsigned char b2 = ref[++zz];
							int idx = ((b1 << 8) + b2);
							
							CPinfo info = constant_pool[idx];
							CPinfo class_name = constant_pool[info.ClassInfo.name_index];
							std::string className = checkClassName(class_name.UTF8Info.bytes);
							
							cout << "checkcast " << jvm_stack.back() << " is a " << className << endl;
						}
						break;
					case OP_instanceof:
						{
							unsigned char b1 = ref[++zz];
							unsigned char b2 = ref[++zz];
							int idx = ((b1 << 8) + b2);
							
							CPinfo info = constant_pool[idx];
							CPinfo class_name = constant_pool[info.ClassInfo.name_index];
							std::string className = checkClassName(class_name.UTF8Info.bytes);
							
							std::string obj = jvm_stack.back();
							jvm_stack.clear();
							jvm_stack.push_back(obj + "instanceof" + className);
						}
						break;
					case OP_monitorenter:
						{
							std::string obj = jvm_stack.back();
							jvm_stack.pop_back();
							BUFF("synchronized(" + obj + ") {\n");
						}
						break;
					case OP_monitorexit:
						{
							BUFF("}\n");
							unsigned char nextOpcode = ref[zz + 1];
							
							if(nextOpcode == 0xa7) // goto
							{
								unsigned char b1 = ref[zz+2];
								unsigned char b2 = ref[zz+3];
								int gotoTarget = static_cast<signed short>((b1 << 8) + b2) + opcodePos + 1;
								while(zz < gotoTarget + 8 - 1)
								{
									zz++;
								}
							}
						}
						break;
					case OP_wide:
						// TODO
						cerr << "wide not implemented, segfault incoming." << endl;
						break;
					case OP_multianewarray:
						{
							unsigned char b1 = ref[++zz];
							unsigned char b2 = ref[++zz];
							int dimension = ref[++zz];
							int idx = ((b1 << 8) + b2);
							
							CPinfo info = constant_pool[idx];
							CPinfo class_name = constant_pool[info.ClassInfo.name_index];
							std::string className = checkClassName(class_name.UTF8Info.bytes);
							
							int p = 0;
							std::string outputType = parseType(className, p);
							std::string type = outputType;
							auto it = std::find(type.begin(), type.end(), '[');
							type.erase(it, type.end());
							
							std::string dimensions;
							while(dimension > 0)
							{
								std::string size = jvm_stack.back();
								jvm_stack.pop_back();
								dimensions = "[" + size + "]" + dimensions;
								dimension--;
							}
							std::string newType = "new " + type + dimensions;
							
							jvm_stack.push_back(newType);
							varTypes[newType] = outputType;
						}
						break;
					case OP_ifnull:
						IF_OPCODE("!= null")
						break;
					case OP_ifnonnull:
						IF_OPCODE("== null")
						break;
					case OP_goto_w:
						// TODO
						cerr << "goto_w not implemented, segfault incoming." << endl;
						break;
					case OP_jsr_w:
						// TODO
						cerr << "jsr_w not implemented, segfault incoming." << endl;
						break;
					case OP_breakpoint:
						cerr << "reserved for breakpoints in Java debuggers; should not appear in any class file." << endl;
						break;
					/* 0xcb to 0xdf are reserved for future use */
					case OP_impdep1:
					case OP_impdep2:
						cerr << "reserved for implementation-dependent operations within debuggers; should not appear in any class file." << endl;
						break;
					default:
						cerr << "Unhandled opcode:" << std::hex << static_cast<int>(c) << endl;
						BUFF("// Unhandled opcode: " + std::to_string(static_cast<int>(c)) + "\n");
				}
			}
			
			for(auto & target : jumpTargets)
			{
				auto pos = std::find(bufferMethod.begin(), bufferMethod.end(), "/*" + std::to_string(target.first) + "*/ ");
				if(pos != bufferMethod.end())
				{
					std::string bufferString;
					for(auto & str : target.second)
					{
						bufferString += str;
					}
					
					bufferMethod.insert(pos, bufferString);
				}
				else
				{
					cerr << "invalid jump target" << endl;
				}
			}
			
			for(auto & str : bufferMethod)
			{
				if(str[0] != '/') // remove the "/*XX*/ " placeholders
					W(str);
			}
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
