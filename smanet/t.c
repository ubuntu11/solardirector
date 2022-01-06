void main(void) {
	unsigned short int x = 65529U;
	short int y = *(short int*)&x;
	unsigned short val = 0x8001;
	short v;

	v = *((short *)&val);
	printf("v: %d\n", v);
	printf("y: %d\n", y);
}
