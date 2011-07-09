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

void prompt()
{
    printf(PROMPT_STRING); 
    fflush(stdout);
}

char* expand_conquer(word_t * wrd)
{
   	word_t * pcrt = wrd;
	char * scrt = malloc(1);
    int len = 1;
    scrt[0] = '\0';
    while(pcrt)
    {
    	const char * x;
    	if (pcrt->expand) {
    		x = getenv(pcrt->string);
    		if (x == NULL)
    			x = "";
    	} else {
    		x = pcrt->string;
    	}
    	len += strlen(x);
    	scrt = realloc(scrt, len);
    	strcat(scrt, x);
    	pcrt = pcrt->next_part;
 	}
 	return scrt;
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
    word_t * wcrt = s->params;
    int param_no = count_params(s);
    char** param_vector = (char**) malloc (sizeof(char*) * (param_no + 2));

    param_vector[0] = strdup(s->verb->string);
    int i = 1;
    
    while (wcrt != NULL) 
    {
    	char * scrt = expand_conquer(wcrt);
 		param_vector[i] = scrt;
		wcrt = wcrt->next_word;
		i++;
	}
	param_vector[i] = NULL;
	return param_vector;
}

void parse_forwards(simple_command_t * s)
{   
    int open_out_flag = (s->io_flags & IO_OUT_APPEND) ? O_APPEND : O_TRUNC;
    int open_err_flag = (s->io_flags & IO_ERR_APPEND) ? O_APPEND : O_TRUNC;
    
    char* out_string = expand_conquer(s->out);
    char* err_string = expand_conquer(s->err);
    char* in_string = expand_conquer(s->in);
    
    if (s->out != NULL && NULL != s->err)
    {
        if (strcmp(out_string, err_string) == 0)
        {
            int fd = open(out_string, O_CREAT | O_WRONLY | open_out_flag, 0644);
            if (-1 == fd) {
                perror("Could not open file.");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
         } 
         else 
         {
            int fd_out = open(out_string, O_CREAT | O_WRONLY | open_out_flag, 0644);
            if (-1 == fd_out) {
                perror("Could not open file.");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            
            int fd_err = open(err_string, O_CREAT | O_WRONLY | open_err_flag, 0644);
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
            int fd_out = open(out_string, O_CREAT | O_WRONLY | open_out_flag, 0644);
            if (-1 == fd_out) {
                perror("Could not open file.");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
        }
        if (s->err != NULL)
        {
            int fd_err = open(err_string, O_CREAT | O_WRONLY | open_err_flag, 0644);
           if (-1 == fd_err) {
              perror("Could not open file.");
              exit(1);
           }
           dup2(fd_err, STDERR_FILENO);
        }
     }
     if (s->in != NULL)
     {
     	int fd_in = open(in_string, O_RDONLY);
     	if (-1 == fd_in) {
        	perror("Could not open file.");
        	exit(1);
       	}
     	dup2(fd_in, STDIN_FILENO);
     }
}

bool is_setenv(word_t * wrd)
{
	word_t * pcrt = wrd;
	if (wrd == NULL)
		return false;
	if (pcrt->string == NULL)
		return false;
	if (pcrt->next_part == NULL)
		return false;
	pcrt = pcrt->next_part;
	if (strcmp(pcrt->string, "=") != 0)
		return false;
	if (pcrt->next_part == NULL)
		return false;
	pcrt = pcrt->next_part;
	if (pcrt->string == NULL)
		return false;
	return true;
}

void setvar (word_t * wrd)
{
	word_t * pcrt = wrd;
	char * left_val  = strdup(pcrt->string);
	char * right_val = strdup(pcrt->next_part->next_part->string);
	setenv(left_val, right_val, 1);
}

void env_parse(simple_command_t * s)
{
	while (is_setenv(s->verb))
	{
  		setvar(s->verb);
   		s->verb = s->params;
   		if (s->params != NULL)
   			s->params = s->params->next_word;
	}
}

int run_command(simple_command_t * s)
{
    if (strcmp(s->verb->string, "quit") == 0 || 0 == strcmp(s->verb->string, "exit"))
        exit(EXIT_SUCCESS);
        
    if (strcmp(s->verb->string, "cd") == 0)
    	chdir(s->params->string);
    	
    char** pp = get_params(s);
    
    env_parse(s);
    	
    pid_t pid;
    int status, ret;
    
    switch(pid = fork())
    {
    	case 0:
        	parse_forwards(s);
        	execvp(s->verb->string, (char *const *)pp);
        	fprintf(stderr, "Execution failed for '%s'\n", s->verb->string);
        	exit(EXIT_FAILURE);
		default:    	
        	ret = waitpid(pid, &status, 0);
        	if (ret == -1)
            	perror("Error on waitpid");
            
        	if (WIFEXITED(status))
        		return WEXITSTATUS(status);
    }
	return 0;
}

int recursive_go(command_t * c)
{   
	pid_t pid, pid_pipe;
	int status, pipefd[2];
	
	switch (c->op) 
	{
	    case OP_NONE:
	       return run_command(c->scmd);

    	case OP_SEQUENTIAL:
    	    recursive_go(c->cmd1);
	        return recursive_go(c->cmd2);
			
		case OP_PARALLEL:
			switch(fork())
			{
				case 0:
					recursive_go(c->cmd1);
					break;
				default:
					recursive_go(c->cmd2);
					break;
			}
			break;
			
		case OP_CONDITIONAL_ZERO:
			switch(recursive_go(c->cmd1))
			{
				case 0:
					return recursive_go(c->cmd2);
				default:
					return 0;
			}
		case OP_CONDITIONAL_NZERO:
			switch(recursive_go(c->cmd1))
			{
				case 0:
					return 0;
				default:
					return recursive_go(c->cmd2);
			}
		case OP_PIPE:
			if (-1 == pipe(pipefd))
				perror("Pipe in recursive_go");
			
			switch(pid = fork())
		    {
		    	case 0:
		    		switch(fork())
		    		{
		    			case 0:
		    				close(pipefd[0]);
		    				dup2(pipefd[1], STDOUT_FILENO);
		        			close(pipefd[1]);
		        			recursive_go(c->cmd1);
							exit(EXIT_SUCCESS);
		    			default:
		    				exit(EXIT_SUCCESS);
		    		}
		    	default:
		    		waitpid(pid, &status, 0);
		    		switch(pid_pipe = fork())
		    		{
		    			case 0:
		    				close(pipefd[1]);
		    				dup2(pipefd[0], STDIN_FILENO);
		    				close(pipefd[0]);
		    				recursive_go(c->cmd2);
		    				exit(EXIT_SUCCESS);
		    			default:
		    				close(pipefd[1]);
		    				close(pipefd[0]);
		    				waitpid	(pid_pipe, &status, 0);
		    				if (WIFEXITED(status))
        						return WEXITSTATUS(status);
        					return 0; 
		    		}
		   }  
		break;
		default:
			assert(false);
	}
	return 0;
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
