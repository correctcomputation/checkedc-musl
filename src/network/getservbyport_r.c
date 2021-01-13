#define _GNU_SOURCE
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* ****************************************************
3C Bounds Inference:  
Patterns present here that we might use as heuristics: 
Usage of pointer parameter in some funky pointer arithmetic 
and involvement of extra parameter indicating length 
(buf += sizeof(char *)-i;)

Solution: 
Change original parameter to something else that allows it
to retain buflen as its length so that all the funky pointer 
arithmetic is handled elswhere 

Exact same problem and solution as gethostbyaddr_r.c 
, gethostbyname2_r.c, and getservbyname_r.c
**************************************************** */

int getservbyport_r(int port, const char *prots,
	struct servent *se, char *buf, size_t buflen, struct servent **res)
{
	int i;
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = port,
	};

	if (!prots) {
		int r = getservbyport_r(port, "tcp", se, buf, buflen, res);
		if (r) r = getservbyport_r(port, "udp", se, buf, buflen, res);
		return r;
	}
	*res = 0;

	/* Align buffer */ 
	// SOLUTION: same drill 
	/* 
	_Array_ptr<char> buf : bounds(buf_ori, buf_ori + buflen) = buf_ori; 
	*/
	i = (uintptr_t)buf & sizeof(char *)-1;
	if (!i) i = sizeof(char *);
	if (buflen < 3*sizeof(char *)-i)
		return ERANGE;
	buf += sizeof(char *)-i;
	buflen -= sizeof(char *)-i;

	if (strcmp(prots, "tcp") && strcmp(prots, "udp")) return EINVAL;

	se->s_port = port;
	se->s_proto = (char *)prots;
	se->s_aliases = (void *)buf;
	buf += 2*sizeof(char *);
	buflen -= 2*sizeof(char *);
	se->s_aliases[1] = 0;
	se->s_aliases[0] = se->s_name = buf;

	switch (getnameinfo((void *)&sin, sizeof sin, 0, 0, buf, buflen,
		strcmp(prots, "udp") ? 0 : NI_DGRAM)) {
	case EAI_MEMORY:
	case EAI_SYSTEM:
		return ENOMEM;
	default:
		return ENOENT;
	case 0:
		break;
	}

	/* A numeric port string is not a service record. */
	if (strtol(buf, 0, 10)==ntohs(port)) return ENOENT;

	*res = se;
	return 0;
}
// FULL SOLUTION 
/*
int getservbyport_r(int port,
	const char *prots : itype(_Nt_array_ptr<const char>),
	struct servent *se : itype(_Ptr<struct servent>),
	char *buf_ori : count(buflen),
	size_t buflen,
	struct servent **res : itype(_Ptr<_Ptr<struct servent>>))
{
	int i;
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = port,
	};

	_Array_ptr<char> buf : bounds(buf_ori, buf_ori + buflen) = buf_ori;
	if (!prots) {
		int r = getservbyport_r(port, "tcp", se, buf, buflen, res);
		if (r) r = getservbyport_r(port, "udp", se, buf, buflen, res);
		return r;
	}
	*res = 0;

	i = (uintptr_t)buf & sizeof(char *)-1;
	if (!i) i = sizeof(char *);
	if (buflen < 3*sizeof(char *)-i)
		return ERANGE;
	buf += sizeof(char *)-i;
	buflen -= sizeof(char *)-i;

	if (strcmp(prots, "tcp") && strcmp(prots, "udp")) return EINVAL;

	se->s_port = port;
	se->s_proto = (char *)prots;
	se->s_aliases = (void *)buf;
	buf += 2*sizeof(char *);
	buflen -= 2*sizeof(char *);
	se->s_aliases[1] = 0;
	se->s_aliases[0] = se->s_name =(char *)buf;

	switch (getnameinfo((void *)&sin, sizeof sin, 0, 0, (char *)buf, buflen,
		strcmp(prots, "udp") ? 0 : NI_DGRAM)) {
	case EAI_MEMORY:
	case EAI_SYSTEM:
		return ENOMEM;
	default:
		return ENOENT;
	case 0:
		break;
	}

	if (strtol((char *)buf, 0, 10)==ntohs(port)) return ENOENT;

	*res = se;
	return 0;
}*/
