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
#include "ClassOutput.h"
#include <fstream>

#define W(c) file << c

void ClassOutput::generate(std::ofstream & file)
{
	if(isPublic)
		W("public ");
	if(isAbstract)
		W("abstract ");
	if(isFinal)
		W("final ");
	if(isInterface)
		W("interface ");
	else if(isEnum)
		W("enum ");
	else
		W("class ");
	
	W(name);
	if(extends != "Object")
	{
		W(" extends ");
		W(extends);
	}
	if(interfaces.size() > 0)
	{
		W(" implements ");
		W(interfaces[0]);
		for(std::size_t i = 1;i < interfaces.size();i++)
		{
			W(", ");
			W(interfaces[i]);
		}
	}
	W(" {\n");
	
	for(MethodOutput m : methods)
	{
		m.thisClass = name;
		m.parentClass = extends;
		m.generate(file);
	}
	
	for(FieldOutput f : fields)
	{
		f.generate(file);
	}
	
	W("}\n");
}
