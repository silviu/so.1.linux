#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>

#include "parser/parser.h"

#ifdef UNICODE
#  error "Unicode not supported in this source file!"
#endif

#define PROMPT_STRING	"> "


void parse_error(const char * str, const int where)
{
	fprintf(stderr, "Parse error near %d: %s\n", where, str);
}

int count_params(simple_command_t * s)
{
    word_t * crt = s->params;
    int i = 0;
    while (crt != NULL) {
        i++;
		crt = crt->next_word;
	}
	return i;
}

char** get_params(simple_command_t * s)
{
    word_t * crt = s->params;
    int param_no = count_params(s);
    char** param_vector = malloc (sizeof(char*) * (param_no + 2));

    param_vector[0] = s->verb->string;
    int i = 1;
    while (crt != NULL) {
        param_vector[i] = crt->string;
		crt = crt->next_word;
		i++;
	}
	param_vector[i] = NULL;
	return param_vector;
}

void run_command(simple_command_t * s)
{
    char** pp = get_params(s);
    if ( fork() == 0)
    {
        if (s->out != NULL)
        {
            int fd = open(s->out->string, O_CREAT|O_RDWR|O_TRUNC, 0644);
            if (-1 == fd)
                printf("Could not open file.");
                dup2(fd, 1);
        }
        execvp(s->verb->string, (char *const *)pp);
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
		    
		    if (root->op == OP_NONE) {
		            run_command(root->scmd);
		    }
		     else { 
		            switch (root->op) {
		                case OP_SEQUENTIAL:
			                break;
		                case OP_PARALLEL:
			                break;
		                case OP_CONDITIONAL_ZERO:
			                break;
		                case OP_CONDITIONAL_NZERO:
			                break;
		                case OP_PIPE:
			                break;
		                default:
			                assert(false);
		            }
		     }
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
