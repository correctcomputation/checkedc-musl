#include <string.h>
#include <resolv.h>

/* RFC 1035 message compression */

/* label start offsets of a compressed domain name s */

/* ****************************************************
3C Bounds Inference:  
Patterns present here that we might use as heuristics: 
1. Using pointer variable as guard for a loop (while (*s))
2. Incrememnting a pointer variable in the body 
	of a loop (s += *s + 1)

Solution: The full solution is below, with some comments
Since s initially has unknown bounds, create an s_tmp with 
length 0, and manually widen s using another temp, s_widened 

	
**************************************************** */
static int getoffs(short *offs, const unsigned char *base, const unsigned char *s)
{ 
	/*
	_Nt_array_ptr<const unsigned char> s_tmp : bounds(s, s + 0) = s;
	_Nt_array_ptr<const unsigned char> s_widened : count(1) = 0; 
	*/
	int i=0;
	for (;;) {
		while (*s & 0xc0) {
			if ((*s & 0xc0) != 0xc0) return 0;
			s = base + ((s[0]&0x3f)<<8 | s[1]); 
			
			/* 
			_Unchecked {
				s_widened = _Assume_bounds_cast<_Nt_array_ptr<const unsigned char>>(s_tmp, count(1));
			} 
			*/
		}
		if (!*s) return i;
		if (s-base >= 0x4000) return 0;
		offs[i++] = s-base;
		s += *s + 1;
	}
} 

// FULL SOLUTION  
// Rough explanation: Key here is to notice the usage of the variable in a loop 
// and create two helper variables to aid in its processing
/*
static int getoffs(_Array_ptr<short> offs : count(128), // Fixed-size, allocated in match.
	_Nt_array_ptr<const unsigned char> base,
	_Nt_array_ptr<const unsigned char> s)
{
	int i=0;
	_Nt_array_ptr<const unsigned char> s_tmp : bounds(s, s + 0) = s;
	_Nt_array_ptr<const unsigned char> s_widened : count(1) = 0;
	for (;;) {
		while (*s_tmp & 0xc0) {
			if ((*s_tmp & 0xc0) != 0xc0) return 0;

			// (*s & 0xc0) != 0 implies *s != 0. So we can widen the bounds of s.
			// Manually widen the bounds for now. The compiler cannot figure it out yet.
			_Unchecked {
				s_widened = _Assume_bounds_cast<_Nt_array_ptr<const unsigned char>>(s_tmp, count(1));
			}
			// Decode offset and calculate the start position of the compressed domain name.
			s_tmp = base + ((s_widened[0]&0x3f)<<8 | s_widened[1]);
		}
		// s reaches the end of the domain name.
		if (!*s_tmp) return i;
		if (s_tmp-base >= 0x4000) return 0;
		offs[i++] = s_tmp-base;
		// Move to the next component.
		s_tmp += *s_tmp + 1;
	}
}
*/

/* label lengths of an ascii domain name s */
static int getlens(unsigned char *lens, const char *s, int l)
{
	int i=0,j=0,k=0;
	for (;;) {
		for (; j<l && s[j]!='.'; j++);
		if (j-k-1u > 62) return 0;
		lens[i++] = j-k;
		if (j==l) return i;
		k = ++j;
	}
}

/* longest suffix match of an ascii domain with a compressed domain name dn */
static int match(int *offset, const unsigned char *base, const unsigned char *dn,
	const char *end, const unsigned char *lens, int nlen)
{
	int l, o, m=0;
	short offs[128];
	int noff = getoffs(offs, base, dn);
	if (!noff) return 0;
	for (;;) {
		l = lens[--nlen];
		o = offs[--noff];
		end -= l;
		if (l != base[o] || memcmp(base+o+1, end, l))
			return m;
		*offset = o;
		m += l;
		if (nlen) m++;
		if (!nlen || !noff) return m;
		end--;
	}
}

int dn_comp(const char *src, unsigned char *dst, int space, unsigned char **dnptrs, unsigned char **lastdnptr)
{
	int i, j, n, m=0, offset, bestlen=0, bestoff;
	unsigned char lens[127];
	unsigned char **p;
	const char *end;
	size_t l = strnlen(src, 255);
	if (l && src[l-1] == '.') l--;
	if (l>253 || space<=0) return -1;
	if (!l) {
		*dst = 0;
		return 1;
	}
	end = src+l;
	n = getlens(lens, src, l);
	if (!n) return -1;

	p = dnptrs;
	if (p && *p) for (p++; *p; p++) {
		m = match(&offset, *dnptrs, *p, end, lens, n);
		if (m > bestlen) {
			bestlen = m;
			bestoff = offset;
			if (m == l)
				break;
		}
	}

	/* encode unmatched part */
	if (space < l-bestlen+2+(bestlen-1 < l-1)) return -1;
	memcpy(dst+1, src, l-bestlen);
	for (i=j=0; i<l-bestlen; i+=lens[j++]+1)
		dst[i] = lens[j];

	/* add tail */
	if (bestlen) {
		dst[i++] = 0xc0 | bestoff>>8;
		dst[i++] = bestoff;
	} else
		dst[i++] = 0;

	/* save dst pointer */
	if (i>2 && lastdnptr && dnptrs && *dnptrs) {
		while (*p) p++;
		if (p+1 < lastdnptr) {
			*p++ = dst;
			*p=0;
		}
	}
	return i;
}
