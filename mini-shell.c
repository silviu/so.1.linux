#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "parser/parser.h"

#ifdef UNICODE
#  error "Unicode not supported in this source file!"
#endif

#define PROMPT_STRING	"> "


void parse_error(const char * str, const int where)
{
	fprintf(stderr, "Parse error near %d: %s\n", where, str);
}



void run_command(word_t * comm, word_t * params)
{
    if (params != NULL) {
        const char* para[] = {comm->string, params->string, NULL};
        if ( fork() == 0)
            execvp(comm->string, (char *const *)para);
    }
    else {
        const char* para[] = {comm->string, NULL};
        if (fork() == 0)
            execvp(comm->string, (char *const *)para);
    }
}


int main(void)
{
	char line[1000];
	command_t *root = NULL;
   // freopen("cmd", "r", stdin);
   printf(PROMPT_STRING); fflush(stdout);
    while (1)
    {
        root = NULL;
	    
	    if (fgets(line, sizeof(line), stdin) == NULL)
	    {
		    fprintf(stderr, "End of file!\n");
		    return EXIT_SUCCESS;
	    }
	    /* we might have not read the entire line... */
	    if (parse_line(line, &root)) 
	    {
		    printf("Command successfully read!\n");
		    if (root == NULL) {
			    printf("Command is empty!\n");
		    } else {
			/* root points to a valid command tree that we can use */
			if (strcmp(line, "quit") == 0)
		        return EXIT_SUCCESS;
		    
		    if (root->op == OP_NONE)
		        run_command(root->scmd->verb, root->scmd->params);
		    }
		}
	    else {
		    /* there was an error parsing the command */
	    }
	
    printf(PROMPT_STRING); fflush(stdout);
	free_parse_memory();
	}
	return EXIT_SUCCESS;
}
