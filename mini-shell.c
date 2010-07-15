#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

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
    char** param_vector = (char**) malloc (sizeof(char*) * (param_no + 2));

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

void parse_forwards(simple_command_t * s)
{   
    int open_out_flag = (s->io_flags & IO_OUT_APPEND) ? O_APPEND : O_TRUNC;
    int open_err_flag = (s->io_flags & IO_ERR_APPEND) ? O_APPEND : O_TRUNC;
    
    if (s->out != NULL && NULL != s->err)
    {
        if (strcmp(s->out->string, s->err->string) == 0)
        {
            int fd = open(s->out->string, O_CREAT | O_WRONLY | open_out_flag, 0644);
            if (-1 == fd) {
                perror("Could not open file.");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
         } 
         else 
         {
            int fd_out = open(s->out->string, O_CREAT | O_WRONLY | open_out_flag, 0644);
            if (-1 == fd_out) {
                perror("Could not open file.");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            
            int fd_err = open(s->err->string, O_CREAT | O_WRONLY | open_err_flag, 0644);
            if (-1 == fd_err) {
               perror("Could not open file.");
               exit(1);
            }
            dup2(fd_err, STDERR_FILENO);
          }
    }
    else
    {
        if (s->out != NULL) 
        {
            int fd_out = open(s->out->string, O_CREAT | O_WRONLY | open_out_flag, 0644);
            if (-1 == fd_out) {
                perror("Could not open file.");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
        }
        if (s->err != NULL)
        {
            int fd_err = open(s->err->string, O_CREAT | O_WRONLY | open_err_flag, 0644);
           if (-1 == fd_err) {
              perror("Could not open file.");
              exit(1);
           }
           dup2(fd_err, STDERR_FILENO);
        }
     }
     if (s->in != NULL)
     {
     	int fd_in = open(s->in->string, O_RDONLY);
     	if (-1 == fd_in) {
        	perror("Could not open file.");
        	exit(1);
       	}
     	dup2(fd_in, STDIN_FILENO);
     }
}

int run_command(simple_command_t * s)
{
    if (strcmp(s->verb->string, "quit") == 0 || 0 == strcmp(s->verb->string, "exit"))
        exit(EXIT_SUCCESS);
        
    if (strcmp(s->verb->string, "cd") == 0)
    	chdir(s->params->string);
   
    char** pp = get_params(s);
    
    pid_t pid;
    pid = fork();
    int status;
    
    if (pid == 0)
    {
        parse_forwards(s);
        execvp(s->verb->string, (char *const *)pp);   
    } else {
    	
        int ret = waitpid(pid, &status, 0);
        if (ret == -1)
            perror("Error on waitpid");
            
        if (WIFEXITED(status))
        	return WEXITSTATUS(status);
        }
	return -2;
}

int recursive_go(command_t * c)
{   
	int exit_status = -1;
	int exi = -1;
	pid_t pid;
	switch (c->op) {
	    case OP_NONE:
	       exit_status = run_command(c->scmd);
	       return exit_status;
	       break;
	        
    	case OP_SEQUENTIAL:
    	    recursive_go(c->cmd1);
	        recursive_go(c->cmd2);
			break;
			
		case OP_PARALLEL:
			pid = fork();
			if (-1 == pid)
				perror("Error on fork in recursive_go");
				
		    if (pid == 0) 
		        recursive_go(c->cmd1);
		    else 
		    	recursive_go(c->cmd2);
			break;
			
		case OP_CONDITIONAL_ZERO:
			exi = recursive_go(c->cmd1);
			if (exi == 0)
				recursive_go(c->cmd2);
			break;
			
		case OP_CONDITIONAL_NZERO:
			exi = recursive_go(c->cmd1);
			if (exi == 1)
				recursive_go(c->cmd2);
			break;
			
		case OP_PIPE:
			break;
			
		default:
			assert(false);
	}

	return exit_status;
}

void prompt()
{
    printf(PROMPT_STRING); 
    fflush(stdout);
}

int main(void)
{
	char line[1000];
	command_t *root = NULL;
    //freopen("cmd", "r", stdin);
    while (1)
    {
        prompt();
        root = NULL;
	    
	    if (fgets(line, sizeof(line), stdin) == NULL)
	    {
		    fprintf(stderr, "End of file!\n");
		    return EXIT_SUCCESS;
	    }
	    /* we might have not read the entire line... */
	    if (parse_line(line, &root)) 
	    {

		    if (root == NULL) {
			    printf("Command is empty!\n");
		    } else {
		    
		    
			/* root points to a valid command tree that we can use */
		    if (root->op == OP_NONE) {
		        run_command(root->scmd);

		    } else { 
		        recursive_go(root);
                }
		    }
		} else {
		    /* there was an error parsing the command */
	    }
	free_parse_memory();
	}
	return EXIT_SUCCESS;
}
