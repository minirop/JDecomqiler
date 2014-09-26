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
#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#define ACC_PUBLIC       0x0001 // all
#define ACC_PRIVATE      0x0002 // fields && methods
#define ACC_PROTECTED    0x0004 // fields && methods
#define ACC_STATIC       0x0008 // fields && methods
#define ACC_FINAL        0x0010 // all
#define ACC_SUPER        0x0020 // classes only
#define ACC_SYNCHRONIZED 0x0020 // methods only
#define ACC_BRIDGE       0x0040 // methods only
#define ACC_VOLATILE	 0x0040 // fields only
#define ACC_VARARGS      0x0080 // methods only
#define ACC_TRANSIENT	 0x0080 // fields only
#define ACC_NATIVE       0x0100 // methods only
#define ACC_INTERFACE    0x0200 // classes (in fact, interfaces) only
#define ACC_ABSTRACT     0x0400 // classes && methods
#define ACC_STRICT       0x0800 // methods only
#define ACC_SYNTHETIC    0x1000 // all, not present in code source
#define ACC_ANNOTATION   0x2000 // classes only
#define ACC_ENUM         0x4000 // classes && fields only

#define ACC_CLASS_MASK   0x8631
#define ACC_METHOD_MASK  0x1DFF
#define ACC_FIELD_MASK   0x50DF

#define T_BOOLEAN 4
#define T_CHAR    5
#define T_FLOAT   6
#define T_DOUBLE  7
#define T_BYTE    8
#define T_SHORT   9
#define T_INT     10
#define T_LONG    11

#endif
