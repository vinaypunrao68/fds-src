/*
 * Copyright (c) 2004-2005, BULL S.A. All rights reserved.
 * Authors: 2004, Laurent Vivier <Laurent.Vivier@bull.net>
 *          2005, Sebastien Dugue <Sebastien.Dugue@bull.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the COPYING.LESSER file at the top level of this source tree.
 */

#ifndef __SYSCALL_X86_H
#define __SYSCALL_X86_H

#include <sys/syscall.h>

/*
 * We need to take care of the case when the library is compiled with fPIC
 * and preserve ebx.
 *
 * The DO_SYSCALL_SAVE_EBX macro takes care of saving and restoring ebx.
 *
 * The registers used with int80 are :
 *
 *	eax	syscall number
 *	ebx	arg1
 *	ecx	arg2
 *	edx	arg3
 *	esi	arg4
 *	edi	arg5
 *
 */

#define DO_SYSCALL_SAVE_EBX					\
		"xchgl	%%ebx, %k2\n\t"				\
		"int	$0x80\n\t"				\
		"xchgl	%%ebx, %k2\n\t"

#define aio_syscall1(type, name, type1, arg1)			\
type name(type1 arg1)						\
{								\
	register unsigned int __retval;				\
	asm volatile (						\
		DO_SYSCALL_SAVE_EBX				\
		: "=a" (__retval)				\
		: "0" (__NR_##name), "c" (arg1)			\
		: "memory", "cc");				\
	return __retval;					\
}

#define aio_syscall2(type, name, type1, arg1, type2, arg2)	\
type name(type1 arg1, type2 arg2)				\
{								\
	register unsigned int __retval;				\
	asm volatile (						\
		DO_SYSCALL_SAVE_EBX				\
		: "=a" (__retval)				\
		: "0" (__NR_##name), "d" (arg1), "c" (arg2)	\
		: "memory", "cc");				\
	return __retval;					\
}

#define aio_syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)	\
type name(type1 arg1, type2 arg2, type3 arg3)			\
{								\
	register unsigned int __retval;				\
	asm volatile (						\
		DO_SYSCALL_SAVE_EBX				\
		: "=a" (__retval)				\
		: "0" (__NR_##name), "S" (arg1), "c" (arg2),	\
		  "d" (arg3)					\
		: "memory", "cc");				\
	return __retval;					\
}

#define aio_syscall4(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)	\
{								\
	register unsigned int __retval;				\
	asm volatile (						\
		DO_SYSCALL_SAVE_EBX				\
		: "=a" (__retval)				\
		: "0" (__NR_##name), "D" (arg1), "c" (arg2),	\
		  "d" (arg3), "S" (arg4)			\
		: "memory", "cc");				\
	return __retval;					\
}

/*
 * Special processing for the 5-argument case as we're running out of registers.
 * ebx is saved on stack
 */
#define aio_syscall5(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5)	\
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) \
{								\
	register unsigned int __retval;				\
	unsigned int __saved_ebx;				\
	asm volatile (						\
		"movl	%%ebx, %2\n\t"				\
		"movl	%3, %%ebx\n\t"				\
		"movl	%1, %%eax\n\t"				\
		"int	$0x80\n\t"				\
		"movl	%2, %%ebx"				\
		: "=a" (__retval)				\
		: "i" (__NR_##name), "m" (__saved_ebx),		\
		  "0" (arg1), "c" (arg2), "d" (arg3),		\
		  "S" (arg4), "D" (arg5)			\
		: "memory", "cc");				\
	return __retval;					\
}




#endif /* __SYSCALL_X86_H */
