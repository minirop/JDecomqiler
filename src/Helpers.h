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
#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <vector>
#include "CPinfo.h"

char letterFromType(std::string type);
std::string typeFromInt(int typeInt);
std::string getName(std::uint16_t index);
std::string removeArray(std::string className);
std::string checkClassName(std::string classname);	
std::vector<std::string> parseSignature(std::string signature);
std::string parseType(std::string signature, int & i);

extern std::vector<CPinfo> constant_pool;

#endif
