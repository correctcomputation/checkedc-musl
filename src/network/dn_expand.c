#include <resolv.h>

/* ****************************************************
3C Bounds Inference:  
Patterns present here that we might use as heuristics: 
1. Incrememnting a pointer within some loop (*dest++) 

Author's note: In the converted file (full solution below), 
it looks like though dest has some funky pointer arithmetic 
going on with it, it remains unchecked.
In fact, if it weren't for the creation of the variable space0
this entire file could just pass for a run of the mill 3C 
single-file conversion, but I can't make sense of why
we need space0 to get everything to work out, 
or why the paraemter dest wasn't made checked or even given an itype
after all of these modifications. 

**************************************************** */
int __dn_expand(const unsigned char *base, const unsigned char *end, const unsigned char *src, char *dest, int space)
{ 

	// SOLUTION (unsure why): 
	const int space0 = space > 254 ? 254 : space; 
	// creates a temp variable for space in order to use 
	// as bounds later on 

	const unsigned char *p = src;
	char *dend, *dbegin = dest; 
	// SOLUTION: 
	/*
	const _Array_ptr<const char> dbegin : count(space0) = dest;
	const _Array_ptr<const char> dend : count(0) = dest + space0; 
	*/
	int len = -1, i, j;
	if (p==end || space <= 0) return -1;
	dend = dest + (space > 254 ? 254 : space);
	/* detect reference loop using an iteration counter */
	for (i=0; i < end-base; i+=2) {
		/* loop invariants: p<end, dest<dend */
		if (*p & 0xc0) {
			if (p+1==end) return -1;
			j = ((p[0] & 0x3f) << 8) | p[1];
			if (len < 0) len = p+2-src;
			if (j >= end-base) return -1;
			p = base+j;
		} else if (*p) {
			if (dest != dbegin) *dest++ = '.';
			j = *p++;
			if (j >= end-p || j >= dend-dest) return -1;
			while (j--) *dest++ = *p++;
		} else {
			*dest = 0;
			if (len < 0) len = p+1-src;
			return len;
		}
	}
	return -1;
} 

// FULL SOLUTION 
/* 
_Checked int __dn_expand(const unsigned char *base : bounds(base, end),
	const unsigned char *end : itype(_Array_ptr<const unsigned char>),
	const unsigned char *src : bounds(src, end),
	char *dest : count(space > 254 ? 254 : space),
	int space)
{
	const int space0 = space > 254 ? 254 : space;
	_Array_ptr<const unsigned char> p : bounds(src, end) = src;
	int len = -1, i, j;
	if (p==end || space <= 0) return -1;
	const _Array_ptr<const char> dbegin : count(space0) = dest;
	const _Array_ptr<const char> dend : count(0) = dest + space0;
	// dst loops through bounds(dbegin, dend).
	_Array_ptr<char> dst : bounds(dbegin, dend) = dest;
	for (i=0; i < end-base; i+=2) {
		if (*p & 0xc0) {
			if (p+1==end) return -1;
			j = ((p[0] & 0x3f) << 8) | p[1];
			if (len < 0) len = p+2-src;
			if (j >= end-base) return -1;
			p = base+j;
		} else if (*p) {
			if (dst != dbegin) *dst++ = '.';
			j = *p++;
			if (j >= end-p || j >= dend-dst) return -1;
			while (j--) *dst++ = *p++;
		} else {
			*dst = 0;
			if (len < 0) len = p+1-src;
			return len;
		}
	}
	return -1;
}
*/

weak_alias(__dn_expand, dn_expand);
