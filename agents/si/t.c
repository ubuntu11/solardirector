
#define _getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
//#define _getlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = ((int)(v) & 0x0F); }
#define _getlong(v) (long) ( ((v)[3]) << 24 | ((v)[2]) << 16 | ((v)[1]) << 8 | ((v)[0]) )


void main(void) {
	unsigned char d[] = { 0x5D, 0xEF, 0xFF, 0xFF };
//	long l = &d;

	printf("l: %d\n", _getlong(d));
}
