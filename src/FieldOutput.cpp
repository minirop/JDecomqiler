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
#include "FieldOutput.h"
#include <fstream>

#define W(c) file << c

void FieldOutput::generate(std::ofstream & file)
{
	if(isPublic)
		W("public ");
	if(isProtected)
		W("protected ");
	if(isPrivate)
		W("private ");
	if(isVolatile)
		W("volatile ");
	if(isTransient)
		W("transient ");
	if(isFinal)
		W("final ");
	if(isStatic)
		W("static ");
	
	W(type);
	W(" ");
	W(name);
	W(";\n");
}
