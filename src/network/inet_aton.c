#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

/* ****************************************************
3C Bounds Inference:  
Note: This may be a weird edge case that we don't need to consider 
involving weirdness with strtoul as opposed to the more 
canonical things we've seen thus far.  

Patterns present here that we might use as heuristics: 
Usage of the address-of operator on z, which can be a checked 
pointer with bounds 


Solution: Manually create new temp variable zp and widen its bounds 
throughout each iteration of the for loop 

TODO: Under review as to whether this is something for 
3C engineers to actually worry about (right now leaning towards NO)

	
**************************************************** */
int __inet_aton(const char *s0, struct in_addr *dest)
{
	const char *s = s0;
	unsigned char *d = (void *)dest;
	unsigned long a[4] = { 0 };
	char *z;
	int i;

	for (i=0; i<4; i++) {
		a[i] = strtoul(s, &z, 0); 
		// SOLUTION 
		/* 
		_Nt_array_ptr<char> zp = 0;
		_Unchecked {
			a[i] = strtoul((char *)s, (char **)&z, 0);
			zp = _Assume_bounds_cast<_Nt_array_ptr<char>>(z, count(0));
		} 
		*/
		if (z==s || (*z && *z != '.') || !isdigit(*s))
			return 0;
		if (!*z) break;
		s=z+1;
	}
	if (i==4) return 0;
	switch (i) {
	case 0:
		a[1] = a[0] & 0xffffff;
		a[0] >>= 24;
	case 1:
		a[2] = a[1] & 0xffff;
		a[1] >>= 16;
	case 2:
		a[3] = a[2] & 0xff;
		a[2] >>= 8;
	}
	for (i=0; i<4; i++) {
		if (a[i] > 255) return 0;
		d[i] = a[i];
	}
	return 1;
}

weak_alias(__inet_aton, inet_aton); 

// FULL SOLUTION 
/*
int __inet_aton(const char *s0 : itype(_Nt_array_ptr<const char>),
	struct in_addr *dest : itype(_Ptr<struct in_addr>))
{
	_Nt_array_ptr<const char> s = s0;
	// Struct in_addr has 32 bytes. Cast it to unsigned char pointer with count(4).
	_Array_ptr<unsigned char> d : count(4) = (_Array_ptr<unsigned char>)dest;
	unsigned long a _Checked[4] = { 0 };
	_Array_ptr<char> z; // Bounds are not known until the call to strtoul.
	int i;

	for (i=0; i<4; i++) {
		// TODO: strtoul does not accept checked pointers yet.
		// Since we cannot use address-of (&) operator on a checked pointer
		// with bounds, we must widen the bounds of z using _Assume_bounds_cast
		// after z is made point to the correct position in s through strtoul.
		_Nt_array_ptr<char> zp = 0;
		_Unchecked {
			a[i] = strtoul((char *)s, (char **)&z, 0);
			zp = _Assume_bounds_cast<_Nt_array_ptr<char>>(z, count(0));
		}
		if (z==s || (*zp && *zp != '.') || !isdigit(*s))
			return 0;
		if (!*zp) break;
		s=(_Nt_array_ptr<const char>)zp+1;
	}
	if (i==4) return 0;
	switch (i) {
	case 0:
		a[1] = a[0] & 0xffffff;
		a[0] >>= 24;
	case 1:
		a[2] = a[1] & 0xffff;
		a[1] >>= 16;
	case 2:
		a[3] = a[2] & 0xff;
		a[2] >>= 8;
	}
	for (i=0; i<4; i++) {
		if (a[i] > 255) return 0;
		d[i] = a[i];
	}
	return 1;
}
*/