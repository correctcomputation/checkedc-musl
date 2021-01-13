#define _GNU_SOURCE
#include <sys/socket.h>
#include <limits.h>
#include <errno.h>
#include "syscall.h"


/* ****************************************************
3C Bounds Inference:  
Patterns present here that we might use as heuristics: 

Non-standard but note how in the full solution, Yahui
decides to create a vlen_temp, presumably in order to make sure 
that the count of msgvec can be vlen without the compiler complaining 

The problem here, of course, is that there don't seem to be any 
obvious triggers, such as the modification of vlen in order 
to warrant the creation of a vlen_temp, so this is probably just 
a hack to get the compiler to be happy, but maybe this is a hack
we ought to familiarize ourselves with if we run into problems in 
the future. IIRC, the current array bounds inference might also 
assign the count of the msgvec to be vlen, and this might be causing 
compiler problems that can only be addressed via the addition of a
temporary variable. 

**************************************************** */

int sendmmsg(int fd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags)
{
#if LONG_MAX > INT_MAX
	int i; 
	// SOLUTION: 
	/*
	unsigned int vlen_tmp = vlen; 
	*/
	if (vlen > IOV_MAX) vlen = IOV_MAX; /* This matches the kernel. */
	if (!vlen) return 0;
	for (i=0; i<vlen; i++) {
		ssize_t r = sendmsg(fd, &msgvec[i].msg_hdr, flags);
		if (r < 0) goto error;
		msgvec[i].msg_len = r;
	}
error:
	return i ? i : -1;
#else
	return syscall_cp(SYS_sendmmsg, fd, msgvec, vlen, flags);
#endif
} 

// FULL SOLUTION: 
/* 
int sendmmsg(int fd,
	struct mmsghdr *msgvec : count(vlen),
	unsigned int vlen, unsigned int flags)
{
#if LONG_MAX > INT_MAX
	int i;
	unsigned int vlen_tmp = vlen;
	if (vlen_tmp > IOV_MAX) vlen_tmp = IOV_MAX; 
	if (!vlen_tmp) return 0;
	for (i=0; i<vlen_tmp; i++) {
		ssize_t r = sendmsg(fd, &msgvec[i].msg_hdr, flags);
		if (r < 0) goto error;
		msgvec[i].msg_len = r;
	}
error:
	return i ? i : -1;
#else
	return syscall_cp(SYS_sendmmsg, fd, msgvec, vlen, flags);
#endif
}
*/
