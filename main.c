#include <stdio.h>
#include <rules.h>
#include <parser.h>

int main()
{
	ListRoot *listroot;
	listroot = configrules("community.rules");
//	listroot = configrules("test-rules-alone");
	precreatearray(listroot);
	test(listroot);
	freeall(listroot);
	printf("End.\n");
	return 0;
}
