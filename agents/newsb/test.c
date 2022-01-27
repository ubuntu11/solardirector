
#include "sb.c"
#include "config.c"

int main(void) {
	sb_session_t *s;

	s = sb_driver.new(0,0,0);
	if (!s) return 1;

        strcpy(s->endpoint,"192.168.1.175");
        strcpy(s->password,"Bgt56yhn!");

	if (sb_driver.open(s)) return 1;
	sb_driver.read(s,0,0);
	sb_driver.close(s);
	return 0;
}
