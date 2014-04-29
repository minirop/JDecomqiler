/*
JDecomqiler

Copyright (c) 2011 <Alexander Roper>

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

#define QT_NO_DEBUG 1

#include <QString>
#include <QVector>
#include <QPair>
#include <QHash>
#include <QDataStream>
#include "ClassOutput.h"

enum {
	CONSTANT_Utf8 = 1,
	CONSTANT_Integer = 3,
	CONSTANT_Float = 4,
	CONSTANT_Long = 5,
	CONSTANT_Double = 6,
	CONSTANT_Class = 7,
	CONSTANT_String = 8,
	CONSTANT_Fieldref = 9,
	CONSTANT_Methodref = 10,
	CONSTANT_InterfaceMethodref = 11,
	CONSTANT_NameAndType = 12
};

struct CPinfo {
	quint8 tag;
	union {
		struct {
			quint16 name_index;
		} ClassInfo;
		struct {
			quint16 class_index;
			quint16 name_and_type_index;
		} RefInfo;
		struct {
			quint16 string_index;
		} StringInfo;
		struct {
			quint32 bytes;
		} IntegerInfo;
		struct {
			float bytes;
		} FloatInfo;
		struct {
			quint64 bytes;
		} BigIntInfo;
		struct {
			quint64 bytes;
		} DoubleInfo;
		struct {
			quint16 name_index;
			quint16 descriptor_index;
		} NameAndTypeInfo;
		struct {
			quint16 length;
			char* bytes;
		} UTF8Info;
	};
};

class ClassFile
{
public:
	ClassFile(QString filename);
	~ClassFile();
	
	QPair<QString, QByteArray> parseAttribute();
	bool parseConstant();
	FieldOutput parseField();
	QString parseInterface();
	MethodOutput parseMethod();
	
	void generate();

private:
	QDataStream stream;
	
	ClassOutput output;
	QVector<CPinfo> constant_pool;
	
	// functions
	void init_init();
	
	QString getName(quint16 index);
	QVector<QString> parseSignature(QString signature);
	QString parseType(QString signature, int & i);
	QString checkClassName(QString classname);
	
	bool istore[4];
	bool skip_return;
	
	QVector<char*> toDelete;
};

#endif
