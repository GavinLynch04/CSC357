#include <stdio.h>
int main() {
int count;
int j;
int c;
count = 0;
while(!feof(stdin)) {
	c = getchar();
	if(c == '\t') {
		for (j = count%8; j <= 7; j++) {
			putchar(' ');
			count++;
		}
	} else if(c == '\n') {
		putchar('\n');
		count = 0;
	} else if(c == '\r') {
		putchar('\r');
		count = 0;
	} else if(c == '\b' && count != 0) {
		count -= 1;
		putchar('\b');
	} else if (!feof(stdin)) {
		putchar(c);
		count++;
	}
}
return 0;
}	
