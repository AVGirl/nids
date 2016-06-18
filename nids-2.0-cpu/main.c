#include <stdio.h>
#include <rules.h>
#include <parser.h>
#include <gpu-match.h>

int main()
{
	ListRoot *listroot;
	listroot = configrules("community.rules");
//	listroot = configrules("test-rules");
	precreatearray(listroot);
	test(listroot);
	gpumatch(listroot);
	freeall(listroot);
	printf("End.\n");
	return 0;
}
