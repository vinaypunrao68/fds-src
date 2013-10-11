#ifndef __SYSCALL_X86_64_H
#define __SYSCALL_X86_64_H

#include <sys/syscall.h>

/*
 * x86_64 syscall macros
 *
 * The registers used are :
 *
 *	rax	syscall number
 *	rdi	arg1
 *	rsi	arg2
 *	rdx	arg3
 *	r10	arg4
 *	r8	arg5
 */


#define aio_syscall1(type, name, type1, arg1)				\
type name(type1 arg1)							\
{									\
	long __retval;							\
	asm volatile("syscall"						\
		     : "=a" (__retval)					\
		     : "0" (__NR_##name), "D" (arg1));			\
	return __retval;						\
}

#define aio_syscall2(type, name, type1, arg1, type2, arg2)		\
type name(type1 arg1, type2 arg2)					\
{									\
	long __retval;							\
	asm volatile("syscall"						\
		     : "=a" (__retval)					\
		     : "0" (__NR_##name), "D" (arg1), "S" (arg2));	\
	return __retval;						\
}

#define aio_syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)	\
type name(type1 arg1, type2 arg2, type3 arg3)				\
{									\
	long __retval;							\
	asm volatile("syscall"						\
		     : "=a" (__retval)					\
		     : "0" (__NR_##name), "D" (arg1), "S" (arg2),	\
		       "d" (arg3)					\
		);							\
	return __retval;						\
}

#define aio_syscall4(type, name, type1, arg1, type2, arg2, type3, arg3,	\
		     type4, arg4)					\
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)		\
{									\
	long __retval;							\
	asm volatile("movq %5,%%r10 ; syscall"				\
		     : "=a" (__retval)					\
		     : "0" (__NR_##name), "D" (arg1), "S" (arg2),	\
		       "d" (arg3), "g" (arg4)				\
		);							\
	return __retval;						\
}

#define aio_syscall5(type, name, type1, arg1, type2, arg2, type3, arg3,	\
		     type4, arg4, type5, arg5)				\
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5)	\
{									\
	long __retval;							\
	asm volatile("movq %5,%%r10 ; movq %6,%%r8 ; syscall"		\
		     : "=a" (__retval)					\
		     : "0" (__NR_##name), "D" (arg1), "S" (arg2),	\
		       "d" (arg3), "g" (arg4), "g" (arg5)		\
		);							\
	return __retval;						\
}

#endif /* __SYSCALL_X86_64_H */
