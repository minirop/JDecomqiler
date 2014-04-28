/*
JDecomqiler

Copyright (c) 2011, 2013 <Alexander Roper>

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

class MethodOutput
{
public:
	MethodOutput() :
		isPublic(false), isProtected(false), isPrivate(false), isAbstract(false),
		isFinal(false), isStatic(false), isSynchronized(false), isBridge(false),
		isVarargs(false), isNative(false), isStrict(false)
	{
	}
	
	QString name;
	QString returnType;
	QVector<QString> parametersType;
	QVector< QPair<QString, QByteArray> > attributes;
	bool isPublic, isProtected, isPrivate, isAbstract, isFinal, isStatic;
	bool isSynchronized, isBridge, isVarargs, isNative, isStrict;
};

#endif
