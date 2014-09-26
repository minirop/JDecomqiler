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
#include "Helpers.h"
#include "defines.h"
#include <iostream>
#include <algorithm>

using namespace std;

std::vector<CPinfo> constant_pool;

char letterFromType(std::string type)
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

std::string typeFromInt(int typeInt)
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

std::string getName(std::uint16_t index)
{
	std::string ret_string("*ERROR*");
	
	CPinfo *data = &constant_pool[index];
	if(data->tag == CONSTANT_Utf8)
	{
		ret_string = std::string(data->UTF8Info.bytes);
	}
	
	return ret_string;
}

std::string checkClassName(std::string classname)
{
	std::replace(classname.begin(), classname.end(), '/', '.');
	size_t start_pos = classname.find("java.lang.");
	if(start_pos != std::string::npos)
	{
		classname.replace(start_pos, 10, std::string());
	}
	return classname;
}

std::string removeArray(std::string className)
{
	size_t start_pos;
	while((start_pos = className.find("[]")) != std::string::npos)
	{
		className.replace(start_pos, 2, "_arr");
	}
	
	return className;
}

std::vector<std::string> parseSignature(std::string signature)
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

std::string parseType(std::string signature, int & i)
{
	std::string tmp;
	switch(signature[i])
	{
		case 'B':
			tmp = "byte";
			break;
		case 'C':
			tmp = "char";
			break;
		case 'D':
			tmp = "double";
			break;
		case 'F':
			tmp = "float";
			break;
		case 'I':
			tmp = "int";
			break;
		case 'J':
			tmp = "long";
			break;
		case 'L':
			do
			{
				i++;
				tmp += signature[i];
			} while(signature[i] != ';');
			tmp.pop_back();
			break;
		case 'S':
			tmp = "short";
			break;
		case 'Z':
			tmp = "boolean";
			break;
		case '[':
			i++;
			tmp = parseType(signature, i);
			tmp += "[]";
			break;
		default:
			cerr << "unrecognized parameter '" << signature[i] << "' type in signature" << endl;
			exit(1);
	}
	
	tmp = checkClassName(tmp);
	
	return tmp;
}
