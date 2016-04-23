#include <stdio.h>
#include <ctype.h>

int main()
{
	char *c = "2h5";

	int i = 0;
	char *p;
	p = c;
	while(*p != '\0')
	{
		if(isdigit((int) *p))
			printf("%c\n", *p);
		p = p + 1;
	}

	return 0;
}
