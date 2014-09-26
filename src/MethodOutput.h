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
#ifndef METHODOUTPUT_H
#define METHODOUTPUT_H

#include "CPinfo.h"
#include <string>
#include <tuple>
#include <vector>

class MethodOutput
{
public:
	void generate(std::ofstream & file);
	
	std::string name;
	std::string returnType;
	std::vector<std::string> parametersType;
	std::vector<std::tuple<std::string, std::string>> attributes;
	bool isPublic = false,
		 isProtected = false,
		 isPrivate = false,
		 isAbstract = false,
		 isFinal = false,
		 isStatic = false,
		 isSynchronized = false,
		 isBridge = false,
		 isVarargs = false,
		 isNative = false,
		 isStrict = false;
	//
	std::string thisClass;
	std::string parentClass;
};

#endif
