#ifndef __SYSCALL_IA64_H
#define __SYSCALL_IA64_H

#include <sys/syscall.h>

#define _SYMSTR(str)	#str
#define SYMSTR(str)	_SYMSTR(str)

#define __ia64_raw_syscall(name) 					\
  __asm__ (".text\n"							\
	   ".globl " SYMSTR(name) "\n"					\
	   SYMSTR(name) ":\n"						\
	   "	mov r15=" SYMSTR( __NR_ ## name ) "\n"			\
	   "	break 0x100000\n"					\
	   "	;;\n"							\
	   "	cmp.eq p6,p0=-1,r10\n"					\
	   "	;;\n"							\
	   "	(p6) sub r8=0,r8\n"					\
	   "	br.ret.sptk.few b0\n"					\
	   ".size " SYMSTR(name) ", . - " SYMSTR(name) "\n"		\
	   ".endp " SYMSTR(name) "\n"					\
	);

#define aio_syscall0(type, name)					\
  extern type name(void);						\
  __ia64_raw_syscall(name);

#define aio_syscall1(type, name, type1, arg1)				\
  extern type name(type1 arg1);						\
  __ia64_raw_syscall(name);

#define aio_syscall2(type, name, type1, arg1, type2, arg2)		\
  extern type name(type1 arg1, type2 arg2);				\
  __ia64_raw_syscall(name);

#define aio_syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)	\
  extern type name(type1 arg1, type2 arg2, type3 arg3);			\
  __ia64_raw_syscall(name);

#define aio_syscall4(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4)	\
  extern type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4);	\
  __ia64_raw_syscall(name);

#define aio_syscall5(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5)	\
  extern type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5);	\
  __ia64_raw_syscall(name);


#endif /* __SYSCALL_IA64_H */
