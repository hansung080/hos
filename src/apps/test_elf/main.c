int g_num = 3;

void add(void) {
	g_num = 4;
}

void _start(void) {
	add();
	return;
}