#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <rules.h>

#define MAX_CH_BUF 10000

void printAcArray(RuleSetRoot *rsr)
{
	int i;
	for(i = 0; i < 256; i++)
	{
		if(rsr->acArray[i] != NULL)
		{
			printf("Ch: %c, contId: %d\n", (char)i, rsr->contId);
			printAcArray(rsr->acArray[i]);
		}
	}
	if(rsr->contId != -1)
	{
		printf("failure: %6d >> %6d\n", rsr->contId, rsr->acArray[256]->contId);
	}
	else printf("failure -- root\n");
}

void printAcNodeTree(AcNode *acNode, int i)
{
	while(acNode != NULL)
	{
		printf("depth: %4d, contId: %4d; pattId: %4d; failId: %4d; root: %4d; str: %s\n", i, acNode->contId, acNode->pattId, acNode->failNode->contId, acNode->root, acNode->str);
		printAcNodeTree(acNode->chdNode, i + 1);
		acNode = acNode->broNode;
	}
}

void test(ListRoot *listroot)
{
	RuleTreeRoot *rtr;
	RuleSetRoot *rsr;
	rtr = listroot->TcpListRoot->prmGeneric;
	rsr = rtr->rsr;
	
	printf("\n\nTest...\n");
	printf("Printf acArray...\n");
	printf("Printf failure...\n");
//	printAcArray(rsr);
	printf("Printf contPattMatch...\n");
//	printAcNodeTree(rtr->contPattMatch, 0);

	printf("Test End.\n");
}

void buildACarray(RuleSetRoot *rsr, OptFpList *fplist)
{
	// goto function
	int state = 0, i = 0;
	char ch;

	while(1)
	{
		ch = fplist->context[i];
		if(ch == '\0') break;
		else if(rsr->acArray[(int)ch] == NULL) break;
		else
		{
			rsr = rsr->acArray[(int)ch];
			i++;
		}
	}

	if(ch != '\0')
	{
		while(ch != '\0')
		{
			newState++;
			rsr->acArray[(int)ch] = (RuleSetRoot *)malloc(sizeof(RuleSetRoot));	
			rsr = rsr->acArray[(int)ch];
			int j;
			for(j = 0; j < 257; j++) rsr->acArray[j] = NULL;
			rsr->contId = -1;
			ch = fplist->context[++i];
		}
		rsr->contId = fplist->index;
	}
	else
	{
		fplist->index = rsr->contId;
	}
}

/*void buildfailfunc(RuleSetRoot *rsr)
{
	int tmp_queue[MAX_STATE];
	int flag_queue = -1;
	int tail_queue = -1;
	int state = 0;
	int i;

	memset(tmp_queue, 0, MAX_STATE * sizeof(int)); 
	for(i = 0; i < 256; i++)
	{
		if(rsr->acArray[state][i] != 0)
		{ printf("processing the queue\n"); // For Debug
			tail_queue++;
			tmp_queue[tail_queue] = rsr->acArray[state][i] & 0x0ffff;
		}
	}

	while(tail_queue == flag_queue)
	{
		flag_queue++;
		state = tmp_queue[flag_queue];
		for(i = 0; i < 256; i++)
		{
			if(rsr->acArray[state][i] != 0)
			{ printf("processing the queue\n"); // For Debug
				tail_queue++;
				tmp_queue[tail_queue] = rsr->acArray[state][i] & 0x0ffff;
				int tmp_state = 0;
				tmp_state = rsr->failure[state];
				while(rsr->acArray[tmp_state][i] == 0) tmp_state = rsr->failure[tmp_state];

				rsr->failure[tmp_queue[tail_queue]] = rsr->acArray[tmp_state][i];
			}
		}
	}
}*/

void buildfailfunc(RuleSetRoot *rsr)
{
	RuleSetRoot *tmp_acNode;
	AcQueue *flag_ac = NULL;
	AcQueue *tail_ac = NULL;
	AcQueue *tmp_ac;
	AcQueue *tmp;

	int i;
	for(i = 0; i < 256; i++) // The upper bound is 256!
	{
		tmp_acNode = rsr->acArray[i];
		if(tmp_acNode != NULL)
		{
			tmp_acNode->acArray[256] = rsr;
			flag_ac = (AcQueue *)malloc(sizeof(AcQueue));
			flag_ac->ac = tmp_acNode;
			flag_ac->next = NULL;
			tail_ac = flag_ac;
			break;
		}
	}
	for(i = i + 1; i < 256; i++)
	{
		tmp_acNode = rsr->acArray[i];
		if(tmp_acNode != NULL)
		{
			tmp_acNode->acArray[256] = rsr;
			tmp_ac = (AcQueue *)malloc(sizeof(AcQueue));
			tmp_ac->ac = tmp_acNode;
			tmp_ac->next = NULL;
			tail_ac->next = tmp_ac;
			tail_ac = tmp_ac;
		}
	}

	while(flag_ac != NULL) // flag_ac is the top of the AcQueue.
	{
		tmp = flag_ac;
		flag_ac = flag_ac->next;
		//tmp_acNode = tmp->ac->chdNode;
		int i;
		for(i = 0; i < 256; i++) // add the original flag_ac(tmp->ac)'s child nodes and find the failure node of each child(tmp_acNode)
		{
			tmp_acNode = ((RuleSetRoot *)tmp->ac)->acArray[i];
			if(tmp_acNode != NULL)
			{
				// failure function
				RuleSetRoot *upper_fail = ((RuleSetRoot *)tmp->ac)->acArray[256];
				while(1)
				{
					RuleSetRoot *upper_acNode;
					int j;
					for(j = 0; j < 256; j++)
					{
						upper_acNode = upper_fail->acArray[j];
						if(upper_acNode != NULL && upper_acNode->contId == tmp_acNode->contId) break;
					}
					if(j != 256)
					{
						tmp_acNode->acArray[256] = upper_acNode; // tmp_acNode is the child node.
						break;
					}
					if(upper_fail != rsr) upper_fail = upper_fail->acArray[256]; // -1 is not root.
					else
					{
						tmp_acNode->acArray[256] = upper_fail;
						break;
					}
				}
				tmp_ac = (AcQueue *)malloc(sizeof(AcQueue));
				tmp_ac->ac = tmp_acNode;
				tmp_ac->next = NULL;
				tail_ac->next = tmp_ac;
				tail_ac = tmp_ac;
			}
		}
		// free the tmp(the original flag_ac) of AcQueue. Not the AcNode!
		free(tmp);
	}
}

AcNode *buildContPattMatch(AcNode *acNode, OptFpList *fplist)
{
	int flag = 0;
	AcNode *tmp = acNode->chdNode;
	while(tmp != NULL)
	{
		flag = 1;
		if(tmp->contId == fplist->index)
		{
			acNode = tmp;
			return;
		}
		else tmp = tmp->broNode;
	}

	AcNode *tmp_acNode;
	tmp_acNode = (AcNode *)malloc(sizeof(AcNode));
	tmp_acNode->str = fplist->context;
	tmp_acNode->contId = fplist->index;
	tmp_acNode->chdNode = NULL;
	tmp_acNode->broNode = NULL;
	tmp_acNode->failNode = NULL;
	tmp_acNode->pattId = 0;
	tmp_acNode->root = -1;
	if(flag == 0)
	{
		acNode->chdNode = tmp_acNode;
		acNode = acNode->chdNode;
	}
	else
	{
		tmp = acNode->chdNode->broNode;
		acNode->chdNode->broNode = tmp_acNode;
		tmp_acNode->broNode = tmp;
		acNode = tmp_acNode;
	}
	return acNode;
}

void buildfailContPattMatch(AcNode *acNode)
{
	AcNode *tmp_acNode;
	tmp_acNode = acNode;
	AcQueue *flag_ac = NULL;
	AcQueue *tail_ac = NULL;
	AcQueue *tmp_ac;
	AcQueue *tmp;

	tmp_acNode = acNode->chdNode;
	if(tmp_acNode != NULL)
	{
		tmp_acNode->failNode = acNode;
		flag_ac = (AcQueue *)malloc(sizeof(AcQueue));
		flag_ac->ac = tmp_acNode;
		flag_ac->next = NULL;
		tail_ac = flag_ac;
		tmp_acNode = tmp_acNode->broNode;
	}
	while(tmp_acNode != NULL)
	{
		tmp_acNode->failNode = acNode;
		tmp_ac = (AcQueue *)malloc(sizeof(AcQueue));
		tmp_ac->ac = tmp_acNode;
		tmp_ac->next = NULL;
		tail_ac->next = tmp_ac;
		tail_ac = tmp_ac;
		tmp_acNode = tmp_acNode->broNode;
	}

	while(flag_ac != NULL) // flag_ac is the top of the AcQueue.
	{
		tmp = flag_ac;
		flag_ac = flag_ac->next;
		tmp_acNode = ((AcNode *)tmp->ac)->chdNode;
		while(tmp_acNode != NULL) // add the original flag_ac(tmp->ac)'s child nodes and find the failure node of each child(tmp_acNode)
		{
			// failure function
			AcNode *upper_fail = ((AcNode *)tmp->ac)->failNode;
			while(1)
			{
				AcNode *upper_acNode = upper_fail->chdNode;
				while(upper_acNode != NULL)
				{
					if(upper_acNode->contId == tmp_acNode->contId) break;
					upper_acNode = upper_acNode->broNode;
				}
				if(upper_acNode != NULL)
				{
					tmp_acNode->failNode = upper_acNode; // tmp_acNode is the child node.
					break;
				}
				if(upper_fail->root == -1) upper_fail = upper_fail->failNode; // -1 is not root.
				else
				{
					tmp_acNode->failNode = upper_fail;
					break;
				}
			}
			tmp_ac = (AcQueue *)malloc(sizeof(AcQueue));
			tmp_ac->ac = tmp_acNode;
			tmp_ac->next = NULL;
			tail_ac->next = tmp_ac;
			tail_ac = tmp_ac;
			tmp_acNode = tmp_acNode->broNode;
		}
		// free the tmp(the original flag_ac) of AcQueue. Not the AcNode!
		free(tmp);
	}
}

void createarray(RuleTreeRoot *treeroot)
{
	if(treeroot->rtn == NULL) return;
	else
	{	printf("\n\n######Create array...\n");
		RuleSetRoot *rsr = (RuleSetRoot *)malloc(sizeof(RuleSetRoot));
		int i;
		for(i = 0; i < 257; i++) rsr->acArray[i] = NULL;	
		rsr->contId = -1;	
		treeroot->rsr = rsr;

		treeroot->contPattMatch = (AcNode *)malloc(sizeof(AcNode));
		treeroot->contPattMatch->str = "";
		treeroot->contPattMatch->contId = -1; // The root of ContPattMatch List.
		treeroot->contPattMatch->chdNode = NULL;
		treeroot->contPattMatch->broNode = NULL;
		treeroot->contPattMatch->failNode = treeroot->contPattMatch;
		treeroot->contPattMatch->pattId = 0; // No pattern here.
		treeroot->contPattMatch->root = 1; // true = 1; root = 1 

		newState = 0;
		RuleTreeNode *treenode;
		treenode = treeroot->rtn;
		while(treenode != NULL)
		{//printf("treenode\n");
			OptTreeNode *optnode;
			optnode = treenode->down;
			while(optnode != NULL)
			{//printf("optnode\n");
				int pattId = optnode->evalIndex;
				AcNode *tmp_acNode;
				tmp_acNode = treeroot->contPattMatch;
				OptFpList *fplist;
				fplist = optnode->opt_func;
				while(fplist != NULL)
				{//printf("fplist\n");
					buildACarray(rsr, fplist);
					tmp_acNode = buildContPattMatch(tmp_acNode, fplist);
					fplist = fplist->next;
				}
				if(tmp_acNode->pattId != 0)
				{
					printf("!!!!!!Error: Different patterns have the same rule options!---%d\n", tmp_acNode->pattId);
				}
				else
				{
					tmp_acNode->pattId = pattId;
				}
				optnode = optnode->next;
			}
			treenode = treenode->right;
		}
		buildfailfunc(rsr);
		buildfailContPattMatch(treeroot->contPattMatch);
	}

	printf("######End of Creating Array. --- Num of State:%d\n", newState);

/*	printf("######Begin of Creating Content Pattern Match AC List...\n");

	if(treeroot->rtn == NULL) return;
	else
	{
		RuleTreeNode *treenode;
		treenode = treeroot->rtn;
		while(treenode != NULL)
		{
			OptTreeNode *optnode;
			optnode = treenode->down;
			while(optnode != NULL)
			{
				OptFpList *fplist;
				fplist = optnode->opt_func;
				while()
			}
		}
	}
	printf("######ENd of Creating Content Pattern Match AC List\n");*/
}

void create(RuleListRoot *rulelistroot)
{
	int i;
	for(i = 0; i < MAX_PORTS; i++)
	{
		createarray(rulelistroot->prmSrcGroup[i]);
		createarray(rulelistroot->prmDstGroup[i]);
	}
	createarray(rulelistroot->prmGeneric);
}

void precreatearray(ListRoot *listroot)
{
	create(listroot->IpListRoot);
	create(listroot->TcpListRoot);
	create(listroot->UdpListRoot);
	create(listroot->IcmpListRoot);
}

/*
int parseruleheader(char *header)
{
	char *pattern = "[ ]*alert[ ](tcp|udp|icmp)[ ][^ \t\n]+[ ](any|)";

	regex_t reg;
	char errbuf[1024];
	int err, nm = 1; // nm is the num of pattern
	regmatch_t pmatch[nm];

	if(regcomp(&reg, pattern, REG_EXTENDED) < 0)
	{
		regerror(err, &reg, errbuf, sizeof(errbuf));
		printf("!!!!!!Error: compilation of pattern is wrong! --- %s\n", errbuf);
		return 0;
	}

	err = regexec(&reg, header, nm, pmatch, 0);
	if(err == REG_NOMATCH)
	{
		printf("!!!!!!Error: no match in rule header!\n");
		return 0;
	}
	else if(err)
	{
		regerror(err, &reg, errbuf, sizeof(errbuf));
		printf("!!!!!!Error: execuation of matching is wrong! --- %s\n", errbuf);
		return 0;
	}
	else
	{
		printf("------Rule Header Match.\n");
	}

	regfree(&reg);
	return 1;
}
*/

RuleTreeNode *parseruleheader(char *ch_buffer)
{
	char ch;
	char *p;
	char *header;
	header = ch_buffer;

	// Check ALERT part of rule header
	p = strstr(header, ALERT);
	if(p == NULL)
	{
		printf("!!!!!!Error: there is no ALERT in rule header!\n");
		return NULL;
	}
	while(p != header)
	{
		if(*p != ' ' && *p != '\t')
		{
			printf("!!!!!!Error: the beginning of ch_buffer of rule header is illegal!\n");
			return NULL;
		}
		header++;
	}
	
	// Check PROTOCOL part of rule header
	// Note: not include ip
	p = strstr(header, TCP);
	int type = 0;
	if(p == NULL)
	{
		p = strstr(header, UDP);
		type = 1;
	}
	if(p == NULL)
	{
		p = strstr(header, ICMP);
		type = 2;
	}
	header += 5;
	if(p == header + 1 && *header == ' ') header += 1; // space after ALERT
	else 
	{
		printf("!!!!!!Error: the character after ALERT is illegal!\n");
		return NULL;
	}
	if(type == 2) header += 4;
	else header += 3;
	if(*header == ' ') header += 1; // space after PROTOCOL
	else
	{
		printf("!!!!!!Error: the character after PROTOCOL is illegal!\n");
		return NULL;
	}

	int i;
    int hport[MAX_PORTS];
	int lport[MAX_PORTS];
	int direction = -1;
	int flag_hport = 0;
	int flag_lport = 0;
	memset(hport, -1, MAX_PORTS * sizeof(int));
	memset(lport, -1, MAX_PORTS * sizeof(int));
	for(i = 0; i < 2; i++)
	{
		// Check IP part of rule header
		p = strstr(header, " "); // space after IP
		header = p + 1; // Ignore src ip

		// Check PORT part of rule header
		p = header;
		if(*p == 'a' && *(p + 1) == 'n' && *(p + 2) == 'y')
		{
			lport[flag_lport++] = 0;
			p += 3;
		}
		else if(*p == '[')
		{
			p++;
		    int tmp_hport = 0;
			int flag = 0;
			while(*p != ']')
			{
				if(isdigit((int) *p))
				{
					tmp_hport = 10 * tmp_hport + (*p - '0');
					p++;
					while(isdigit((int) *p))
					{
						tmp_hport = 10 * tmp_hport + (*p - '0');
						p++;
					}
				}
				else
				{
					// ignore the form like [variable, num]
					p = strstr(p, ",");
				}
				if(*p == ',')
				{
					flag = -1;				
					p++;
				}
				else if(*p == ':')
				{
					flag = -2;
					if(*(p+1) == ',')
					{
						flag = -3;
						p++;
					}
					p++;
				}
				else flag = -1;
				switch(flag)
				{
					case -1:
						lport[flag_lport++] = tmp_hport;
						break;
					case -2:
						lport[flag_lport++] = tmp_hport;
						lport[flag_lport++] = flag; 
						break;
					case -3:
						lport[flag_lport++] = tmp_hport;
						lport[flag_lport++] = flag;
						break;
					default:
						break;
				}
				tmp_hport = 0;
			}
			p++;
		}
		else if(isdigit((int) *p))
		{
			int tmp_hport = 0;
			tmp_hport = 10 * tmp_hport + (*p - '0');
			p++;
			while(isdigit((int) *p))
			{
				tmp_hport = 10 * tmp_hport + (*p - '0');
				p++;
			}
			lport[flag_lport++] = tmp_hport;
			if(*p == ':')
			{
				p++;
				lport[flag_lport++] = -2;
				tmp_hport = 0;
				if(isdigit((int) *p))
				{
					tmp_hport = 10 * tmp_hport + (*p - '0');
					p++;
					while(isdigit((int) *p))
					{
						tmp_hport = 10 * tmp_hport + (*p - '0');
						p++;
					}
					lport[flag_lport++] = tmp_hport;
				}
			}
		}
		else // PORT is a varialable.
		{
			lport[flag_lport++] = 0;
			p = strstr(header, " ");
		}
		if(i == 0)
		{
			memcpy(hport, lport, flag_lport * sizeof(int));
			flag_hport = flag_lport;
			// reset lport & flag_lport
			memset(lport, -1, flag_lport * sizeof(int));
			flag_lport = 0;
		}
		if(*p != ' ') // space after the PORT number
		{
			if(i == 0) printf("!!!!!!Error: the character after first PORT number is illegal!\n");
			if(i == 1) printf("!!!!!!Error: the character after second PORT number is illegal!\n");
			return NULL;
		}
		if(i == 0) header = p + 1; 
		p = header;

		// Check the direction only when i == 0
		if(i == 0)
		{
			if(*p == '-' && *(p + 1) == '>')
			{
				direction = 1;
			}
			else if(*p == '<' && *(p + 1) == '-')
			{
				direction = 2;
			}
			else if(*p == '<' && *(p + 1) == '>')
			{
				direction = 4;
			}
			else
			{
				printf("!!!!!!Error: the character of DIRECTION is illegal!\n");
				return NULL;
			}
			if(*(p + 2) != ' ') // space after the DIRECTION
			{
				printf("!!!!!!Error: the character after DIRECTION is illegal!\n");
				return NULL;
			}
			else header = p + 3;
		}
	}
	
	printf("############################################\n");
	printf("alert type: %d\n", type);
	printf("direction type: %d\n", direction);
	printf("first port number\t");
	for(i = 0; i < flag_hport; i++) printf("%d\t", hport[i]);
	printf("\nsecond port number\t");
	for(i = 0; i < flag_lport; i++) printf("%d\t", lport[i]);
    printf("\n");

	//configure array hport and array lport as unsigned_hport and unsigned_lport
	uint16_t unsigned_hport[MAX_PORTS];
	uint16_t unsigned_lport[MAX_PORTS];
	memset(unsigned_hport, 0, MAX_PORTS * sizeof(uint16_t));
	memset(unsigned_lport, 0, MAX_PORTS * sizeof(uint16_t));
	int j = 0;
	for(i = 0; i < flag_hport; i++)
	{
		if(hport[i] == 0) ;
		else if(hport[i] == -2 && hport[i + 1] != -1)
		{
			while(unsigned_hport[j - 1] != hport[i + 1] - 1)
				unsigned_hport[j++] = unsigned_hport[j - 1] + 1;	
		}
		else if(hport[i] == -3 || (hport[i] == -2 && hport[i + 1] == -1))
		{
			while(unsigned_hport[j - 1] != (MAX_PORTS - 1))
				unsigned_hport[j++] = unsigned_hport[j - 1] + 1;
		}
		else unsigned_hport[j++] = hport[i];
	}
	printf("first port number\t");
	if(j == 0) printf("%u\t", unsigned_hport[j]);
	for(i = 0; i < j; i++) printf("%u\t", unsigned_hport[i]); 
	j = 0;
	for(i = 0; i < flag_lport; i++)
	{
		if(lport[i] == 0) ;
		else if(lport[i] == -2 && lport[i + 1] != -1)
		{
			while(unsigned_lport[j - 1] != lport[i + 1] - 1)
				unsigned_lport[j++] = unsigned_lport[j - 1] + 1;	
		}
		else if(lport[i] == -3 || (lport[i] == -2 && lport[i + 1] == -1))
		{ 
			while(unsigned_lport[j - 1] != (MAX_PORTS - 1))
				unsigned_lport[j++] = unsigned_lport[j - 1] + 1;
		}
		else unsigned_lport[j++] = lport[i];
	}
	printf("\nsecond port number\t");
	if(j == 0) printf("%u\t", unsigned_lport[j]);
	for(i = 0; i < j; i++) printf("%u\t", unsigned_lport[i]);
	printf("\n");

	RuleTreeNode *rtn = (RuleTreeNode *)malloc(sizeof(RuleTreeNode));
	rtn->type = type;
	memcpy(rtn->hdp, unsigned_hport, MAX_PORTS * sizeof(uint16_t));
	memcpy(rtn->ldp, unsigned_lport, MAX_PORTS * sizeof(uint16_t));
	rtn->flags = direction;
	rtn->right = NULL;
	rtn->down = NULL;

	return rtn;
}

RuleTreeNode *parseruleoption(RuleTreeNode *rtn, char *ch_buffer, int id)
{
	printf("------Start of rule option of %d\n", id);

	OptTreeNode *opt_node;
	opt_node = (OptTreeNode *)malloc(sizeof(OptTreeNode));
	opt_node->opt_func = NULL;
	opt_node->type = rtn->type;
	opt_node->evalIndex = id;
	opt_node->msg = NULL;
	opt_node->next = NULL;
	opt_node->rtn = rtn;
	rtn->down = opt_node;

	char *sym[SYM_NUM];
	int fsym[SYM_NUM];
	sym[0] = "msg";
	sym[1] = "content";
	sym[2] = "depth";
	sym[3] = "offset";
	sym[4] = "within";
	sym[5] = "distance";
	sym[6] = "nocase";
	fsym[0] = MSG;
	fsym[1] = CONTENT;
	fsym[2] = DEPTH;
	fsym[3] = OFFSET;
	fsym[4] = WITHIN;
	fsym[5] = DISTANCE;
	fsym[6] = NOCASE;

	char *p1;
	char *p2;
	char *option;
	option = ch_buffer;
	while(*option != '\0')
	{
		int len;
		char *pChar;
		int flag = 0;
		p1 = strstr(option, ":");
		p2 = strstr(option, ";");
		if(p1 < p2)
		{
			len = p1 - option;
			pChar = (char *)malloc((len + 1) * sizeof(char));
		}
		else
		{
			len = p2 - option;
			pChar = (char *)malloc((len + 1) * sizeof(char));
		}
		memcpy(pChar, option, len * sizeof(char));
		pChar[len] = '\0';
		int i;
		for(i = 0; i < SYM_NUM; i++)
		{
			if(!strcmp(pChar, sym[i])) break; // pattern matching succeed!
		}
		// free(pChar); (1)
		
		if(i == SYM_NUM)
		{
			option = p2 + 1; // jump to the character after ';'
			printf("******Ignore part of rule option: %s\n", pChar);
			free(pChar); // replace free (1)
		}
		else
		{
			free(pChar); // replace free (1)
			
			if(i == 0 || i == 1) // "msg" || "content"
			{
				option = p1 + 2; // suppose the form is name:value without any space
				int exclamation = 0;
				if(*(option - 1) == '!') // for content:!"..."
				{
					option++; 
					exclamation = 1;
				}
				len = p2 - option - 1;
				pChar = (char *)malloc((len + 1) * sizeof(char));
				char *ptr_vertline = NULL;
				ptr_vertline = strstr(option, "|");
				if(ptr_vertline == NULL || (ptr_vertline != NULL && (ptr_vertline - option) > len))
				{
					memcpy(pChar, option, len * sizeof(char));
					pChar[len] = '\0';
				}
				else
				{
					int j = 0, k = 0;
					int flag_vertline = 0;
					while(k < len) // ignore bytecode
					{
						if(option[k] == '|') flag_vertline++;
						else if(option[k] != '|' && flag_vertline % 2 == 0) pChar[j++] = option[k];
						else ;
						k++;
					}
					if(j == 0) pChar[j++] = 't'; // ignore bytecode
					pChar[j] = '\0';
					len = j;
				}

				if(i == 0)
					opt_node->msg = pChar;
				if(i == 1)
				{
					OptFpList *opt_fp;
					opt_fp = (OptFpList *)malloc(sizeof(OptFpList));
					opt_fp->context = pChar;
					opt_fp->index = contIndex;
					opt_fp->depth = 0;
					opt_fp->offset = 0;
					opt_fp->distance = 0;
					opt_fp->within = 0;
					opt_fp->flags = 0;
					opt_fp->next = NULL;

					OptFpList *tmp_ptr_opt_fp;
					tmp_ptr_opt_fp = opt_node->opt_func;
					opt_node->opt_func = opt_fp;
					opt_fp->next = tmp_ptr_opt_fp;

					if(exclamation == 1) opt_node->opt_func->flags += fsym[i];

					contIndex++;
				}

				if(i == 0) printf("msg: %s\n", pChar);
				if(i == 1 && exclamation == 0) printf("content: %s\n", pChar);
				if(i == 1 && exclamation == 1) printf("content:!\" %s\"\n", pChar);
			}
			else if(i == 6) // nocase
			{
				opt_node->opt_func->flags += fsym[i];
				printf("nocase\n");
			}
			else // others
			{
				// change the flags
				opt_node->opt_func->flags += fsym[i];

				// attain the value
				option = p1 + 1; // suppose the for is name:value without any space
				len = p2 - option;
				pChar = (char *)malloc((len + 1) * sizeof(char));
				memcpy(pChar, option, len * sizeof(char));
				pChar[len] = '\0';
				int j;
				int tmp = 0;

				if(pChar[0] == '!') j = 1; // for the case like name:!value
				else j = 0;
				for(; pChar[j] != '\0'; j++)
					tmp = 10 * tmp + (pChar[j] - '0');
				if(pChar[0] == '!') tmp = -tmp;

				switch(i)
				{
					case 2: // depth
						opt_node->opt_func->depth = tmp;
						break;
					case 3: // offset
						opt_node->opt_func->offset = tmp;
						break;
					case 4: // within
						opt_node->opt_func->within = tmp;
						break;
					case 5: // distance
						opt_node->opt_func->distance = tmp;
						break;
					default:
						printf("!!!!!!Error: i of opt_func is illegal -- %d\n", i);
						return NULL;
				}

				switch(i)
				{
					case 2: // depth
						printf("depth: %d\n", tmp);
						break;
					case 3: // offset
						printf("offset: %d\n", tmp);
						break;
					case 4: // within
						printf("within: %d\n", tmp);
						break;
					case 5: // distance
						printf("distance: %d\n", tmp);
						break;
					default:
						printf("!!!!!!Error: i of opt_func is illegal -- %d\n", i);
						return NULL;
				}
				
			}
			option = p2 + 1;
		}
		if(*option == ' ') option++;
	}
	return rtn;
}

void assemblelistroot(ListRoot *listroot, RuleTreeNode *rtn)
{
	// in terms of type of PROTOCOL, put parsed rule in the listroot
	RuleListRoot *rulelistroot;
	switch(rtn->type)
	{
		case 0:
			rulelistroot = listroot->TcpListRoot;
			break;
		case 1:
			rulelistroot = listroot->UdpListRoot;
			break;
		case 2:
			rulelistroot = listroot->IcmpListRoot;
			break;
		default:
			printf("!!!!!!Error: alert type is not Tcp or Udp or Icmp!\n");
			break;
	}

	RuleTreeRoot *pRoot;
	RuleTreeNode *ptr_rtn;
	int flag = 0;
//	printf("%d %d\n", rtn->flags&0x00000007&1, (rtn->flags&0x00000007)==0x01);printf("%d\n", 1&0x1 == 1);
	if(rtn->flags & 0x00000007 & 0x01 || rtn->flags & 0x00000007 & 0x04) // direction is "->" || "<>"
	{
		if(rtn->hdp[0] == 0 && rtn->ldp[0] != 0 && rtn->ldp[1] == 0) 
		{
			pRoot = rulelistroot->prmDstGroup[rtn->ldp[0]];
		}
		else if(rtn->hdp[0] != 0 && rtn->hdp[1] == 0 && rtn->ldp[0] == 0) 
		{
			pRoot = rulelistroot->prmSrcGroup[rtn->hdp[0]];
		}
		else if(rtn->hdp[0] != 0 && rtn->hdp[1] == 0 && rtn->ldp[0] != 0 && rtn->ldp[1] == 0) 
		{
			pRoot = rulelistroot->prmDstGroup[rtn->ldp[0]];
			flag = 1;
		}
		else
			pRoot = rulelistroot->prmGeneric;
		
		ptr_rtn = pRoot->rtn;
		pRoot->rtn = rtn;
		rtn->right = ptr_rtn;

		if(flag == 1)
		{
			pRoot = rulelistroot->prmSrcGroup[rtn->hdp[0]];

			ptr_rtn = pRoot->rtn;
			pRoot->rtn = rtn;
			rtn->right = ptr_rtn;
		}
	}
	if(rtn->flags & 0x00000007 & 0x02 || rtn->flags & 0x00000007 & 0x04) // direction is "<-" || "<>"
	{
		if(rtn->hdp[0] == 0 && rtn->ldp[0] != 0 && rtn->ldp[1] == 0) 
		{
			pRoot = rulelistroot->prmSrcGroup[rtn->ldp[0]];
		}
		else if(rtn->hdp[0] != 0 && rtn->hdp[1] == 0 && rtn->ldp[0] == 0) 
		{
			pRoot = rulelistroot->prmDstGroup[rtn->hdp[0]];
		}
		else if(rtn->hdp[0] != 0 && rtn->hdp[1] == 0 && rtn->ldp[0] != 0 && rtn->ldp[1] == 0) 
		{
			pRoot = rulelistroot->prmDstGroup[rtn->ldp[0]];
			flag = 1;
		}
		else
			pRoot = rulelistroot->prmGeneric;
		
		ptr_rtn = pRoot->rtn;
		pRoot->rtn = rtn;
		rtn->right = ptr_rtn;

		if(flag == 1)
		{
			pRoot = rulelistroot->prmSrcGroup[rtn->hdp[0]];

			ptr_rtn = pRoot->rtn;
			pRoot->rtn = rtn;
			rtn->right = ptr_rtn;
		}
	}
}

int readfile(ListRoot* listroot, char *pFName)
{
	FILE *pFile = fopen(pFName, "r");
	
	int lNum = 0;
	int patternNum = 0;
	char ch;
	char ch_buffer[MAX_CH_BUF];
	int ch_buf_num = 0;
	int hd_op_flag = 0;

	fscanf(pFile, "%c", &ch);
	while(!feof(pFile))
	{
		lNum++;

		if(ch == '#')
		{
			while(ch != '\n')
			{
				fscanf(pFile, "%c", &ch);
			}
			printf("------Ignore Line %d\n", lNum);
		}
		else
		{
			patternNum++;
			RuleTreeNode *return_rtn;
			hd_op_flag = 0;
			ch_buf_num = 0;
			memset(ch_buffer, 0, MAX_CH_BUF * sizeof(char));
			while(1)
			{
			    if(ch_buf_num > MAX_CH_BUF)
			    {
				    printf("!!!!!!Error: ch_buffer has overflowed!\n");
					return 0;
				}

				if(hd_op_flag == 0) // for rule header part
				{
					switch(ch)
					{
						case '\n':
							printf("------Ignore Line %d\n", lNum);
							hd_op_flag = -1;
							break;
						case '(':
							hd_op_flag = 1;
							break;
						default:
							ch_buffer[ch_buf_num++] = ch;
							break;
					}
				}
				else if(hd_op_flag == 2) // for rule option part
				{
					switch(ch)
					{
						case '\n':
							hd_op_flag = 3;
							break;
						case '(': // suppose there is no '(' | or ')' in content 
							ch_buffer[ch_buf_num++] = ch;
							break;
						case ')':
							ch_buffer[ch_buf_num++] = ch;
							break;
						default:
							ch_buffer[ch_buf_num++] = ch;
							break;
					}
				}

				if(hd_op_flag == 1)
				{
					// parse ch_buffer for rule header part
					return_rtn = parseruleheader(ch_buffer); 
					printf("%s\n", ch_buffer);
					
					hd_op_flag = 2;
					ch_buf_num = 0;
					memset(ch_buffer, 0, MAX_CH_BUF * sizeof(char));
				}
				else if(hd_op_flag == 3)
				{ 
					// parse ch_buffer for rule option part
					ch_buffer[ch_buf_num - 1] = '\0'; // eliminate the final ')'
					return_rtn = parseruleoption(return_rtn, ch_buffer, patternNum);
					assemblelistroot(listroot, return_rtn);
					printf("%s\n", ch_buffer);
					hd_op_flag = -1;
				}
				
				if(hd_op_flag == -1) break;

				fscanf(pFile, "%c", &ch);
			}
			printf("******Finish Line %d for pattern %d\n", lNum, patternNum);
		}
		fscanf(pFile, "%c", &ch);
	}
	fclose(pFile);
	return 1;
}

void freeoptfplist(OptFpList *fp)
{
	OptFpList *ptr_fp;

	while(fp != NULL)
	{
		ptr_fp = fp;
		fp = ptr_fp->next;
		free(ptr_fp);
	}
}
void freeopttreenode(OptTreeNode *opt)
{
	OptTreeNode *ptr_opt;
	while(opt != NULL)
	{
		ptr_opt = opt;
		opt = ptr_opt->next;
		
		if(ptr_opt->opt_func != NULL) freeoptfplist(ptr_opt->opt_func);

		free(ptr_opt);
	}
}

void freerulesetroot(RuleSetRoot *rsr)
{
}

void freeruletreenode(RuleTreeNode *rtn)
{
	RuleTreeNode *ptr_rtn;
	while(rtn != NULL)
	{
		ptr_rtn = rtn;
		rtn = ptr_rtn->right;

		if(ptr_rtn->down != NULL) freeopttreenode(ptr_rtn->down);

		free(ptr_rtn);
	}
}

void freerulelistroot(RuleListRoot *rulelistroot)
{
	RuleTreeRoot *ptr_treeroot;
	
	int i;
	for(i = 0; i < MAX_PORTS; i++)
	{
		ptr_treeroot = rulelistroot->prmSrcGroup[i];
		if(ptr_treeroot->rtn != NULL) freeruletreenode(ptr_treeroot->rtn);
		if(ptr_treeroot->rsr != NULL) freerulesetroot(ptr_treeroot->rsr);
		free(ptr_treeroot);

		ptr_treeroot = rulelistroot->prmDstGroup[i];
		if(ptr_treeroot->rtn != NULL) freeruletreenode(ptr_treeroot->rtn);
		if(ptr_treeroot->rsr != NULL) freerulesetroot(ptr_treeroot->rsr);
		free(ptr_treeroot);
	}

	ptr_treeroot = rulelistroot->prmGeneric;
	if(ptr_treeroot->rtn != NULL) freeruletreenode(ptr_treeroot->rtn);
	if(ptr_treeroot->rsr != NULL) freerulesetroot(ptr_treeroot->rsr);
	
	free(ptr_treeroot);
}

void freeall(ListRoot *listroot)
{
	RuleListRoot *ptr_rulelistroot;
	
	ptr_rulelistroot = listroot->IpListRoot;
	if(ptr_rulelistroot != NULL) freerulelistroot(ptr_rulelistroot);
	free(ptr_rulelistroot);
	
	ptr_rulelistroot = listroot->TcpListRoot;
	if(ptr_rulelistroot != NULL) freerulelistroot(ptr_rulelistroot);
	free(ptr_rulelistroot);
	
	ptr_rulelistroot = listroot->UdpListRoot;
	if(ptr_rulelistroot != NULL) freerulelistroot(ptr_rulelistroot);
	free(ptr_rulelistroot);
	
	ptr_rulelistroot = listroot->IcmpListRoot;
	if(ptr_rulelistroot != NULL) freerulelistroot(ptr_rulelistroot);
	free(ptr_rulelistroot);

	free(listroot);
}

ListRoot *configrules(char *filename)
{
	ListRoot *listroot = (ListRoot *)malloc(sizeof(ListRoot));
	listroot->IpListRoot = (RuleListRoot *)malloc(sizeof(RuleListRoot));
	listroot->TcpListRoot = (RuleListRoot *)malloc(sizeof(RuleListRoot));
	listroot->UdpListRoot = (RuleListRoot *)malloc(sizeof(RuleListRoot));
	listroot->IcmpListRoot = (RuleListRoot *)malloc(sizeof(RuleListRoot));

	int i;
	RuleListRoot *rulelistroot;
	rulelistroot = listroot->IpListRoot;
	for(i = 0; i < MAX_PORTS; i++)
	{
		rulelistroot->prmDstGroup[i] = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
		rulelistroot->prmDstGroup[i]->rtn = NULL;
		rulelistroot->prmDstGroup[i]->rsr = NULL;
		rulelistroot->prmDstGroup[i]->contPattMatch = NULL;

		rulelistroot->prmSrcGroup[i] = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
		rulelistroot->prmSrcGroup[i]->rtn = NULL;
		rulelistroot->prmSrcGroup[i]->rsr = NULL;
		rulelistroot->prmSrcGroup[i]->contPattMatch = NULL;
	}

	rulelistroot = listroot->TcpListRoot;
	for(i = 0; i < MAX_PORTS; i++)
	{
		rulelistroot->prmDstGroup[i] = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
		rulelistroot->prmDstGroup[i]->rtn = NULL;
		rulelistroot->prmDstGroup[i]->rsr = NULL;
		rulelistroot->prmDstGroup[i]->contPattMatch = NULL;

		rulelistroot->prmSrcGroup[i] = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
		rulelistroot->prmSrcGroup[i]->rtn = NULL;
		rulelistroot->prmSrcGroup[i]->rsr = NULL;
		rulelistroot->prmSrcGroup[i]->contPattMatch = NULL;
	}

	rulelistroot = listroot->UdpListRoot;
	for(i = 0; i < MAX_PORTS; i++)
	{
		rulelistroot->prmDstGroup[i] = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
		rulelistroot->prmDstGroup[i]->rtn = NULL;
		rulelistroot->prmDstGroup[i]->rsr = NULL;
		rulelistroot->prmDstGroup[i]->contPattMatch = NULL;

		rulelistroot->prmSrcGroup[i] = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
		rulelistroot->prmSrcGroup[i]->rtn = NULL;
		rulelistroot->prmSrcGroup[i]->rsr = NULL;
		rulelistroot->prmSrcGroup[i]->contPattMatch = NULL;
	}

	rulelistroot = listroot->IcmpListRoot;
	for(i = 0; i < MAX_PORTS; i++)
	{
		rulelistroot->prmDstGroup[i] = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
		rulelistroot->prmDstGroup[i]->rtn = NULL;
		rulelistroot->prmDstGroup[i]->rsr = NULL;
		rulelistroot->prmDstGroup[i]->contPattMatch = NULL;

		rulelistroot->prmSrcGroup[i] = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
		rulelistroot->prmSrcGroup[i]->rtn = NULL;
		rulelistroot->prmSrcGroup[i]->rsr = NULL;
		rulelistroot->prmSrcGroup[i]->contPattMatch = NULL;
	}

	listroot->IpListRoot->prmGeneric = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
	listroot->TcpListRoot->prmGeneric = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
	listroot->UdpListRoot->prmGeneric = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));
	listroot->IcmpListRoot->prmGeneric = (RuleTreeRoot *)malloc(sizeof(RuleTreeRoot));

	RuleTreeRoot *ruletreeroot = listroot->IpListRoot->prmGeneric;
	ruletreeroot->rtn = NULL;
	ruletreeroot->rsr = NULL;
	ruletreeroot->contPattMatch = NULL;

	ruletreeroot = listroot->TcpListRoot->prmGeneric;
	ruletreeroot->rtn = NULL;
	ruletreeroot->rsr = NULL;
	ruletreeroot->contPattMatch = NULL;

	ruletreeroot = listroot->UdpListRoot->prmGeneric;
	ruletreeroot->rtn = NULL;
	ruletreeroot->rsr = NULL;
	ruletreeroot->contPattMatch = NULL;

	ruletreeroot = listroot->IcmpListRoot->prmGeneric;
	ruletreeroot->rtn = NULL;
	ruletreeroot->rsr = NULL;
	ruletreeroot->contPattMatch = NULL;

	// read rule file and configure rules
	if(!readfile(listroot, filename))
	{
		printf("\n\n$$ Error(s)!");
		return NULL;
	}

/*	RuleTreeNode *ptr_node;
	RuleTreeNode *free_ptr_node;
	ptr_node = ruletreeroot->rtn;
	free_ptr_node = ptr_node;
	while(1)
	{
		free_ptr_node = ptr_node;
		if(ptr_node == NULL)
		{
			printf("End! (ptr_node == NULL)\n");
			return 0;
		}
		printf("alert type %d\n", ptr_node->type);
		printf("direction %d\n", ptr_node->flags);
		printf("first port number\t");
		int i = 0;
		while(1)
		{
			if(ptr_node->hdp[i] == -1) break;
			printf("%d\t", ptr_node->hdp[i]);
			i++;
		}
		printf("\nsecond port number\t");
		i = 0;
		while(1)
		{
			if(ptr_node->ldp[i] == -1) break;
			printf("%d\t", ptr_node->ldp[i]);
			i++;
		}
		printf("\n");
		ptr_node = ptr_node->right;
		free(free_ptr_node);
	}*/

	return listroot;
}

int main()
{
	ListRoot *listroot;
	listroot = configrules("community.rules");
	//listroot = configrules("test-rules-2");
	//listroot = configrules("test-rules-2");
	precreatearray(listroot);
	test(listroot);
	freeall(listroot);
	return 0;
}
