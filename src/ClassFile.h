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

#include <fstream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "ClassOutput.h"
#include "CPinfo.h"
#include "Helpers.h"

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
	
	std::vector<char *> toDelete; // find a better way (see (1))
	
	StreamReader stream;
	
	// functions
	std::tuple<std::string, std::string> parseAttribute();
	bool parseConstant();
	FieldOutput parseField();
	std::string parseInterface();
	MethodOutput parseMethod();
};

#endif
