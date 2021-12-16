
function xdprintf(level,format,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z) {
	level = arguments[0];
//	printf("level: %d, debug: %d\n", level, agent.debug);
	if (level <= agent.debug) {
		format = arguments[1];
		printf(format,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z);
	}
}
