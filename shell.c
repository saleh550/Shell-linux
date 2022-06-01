/*********************
* Assignment 2 Code
* SE 317: Operating Systems
* Kinneret Academic College
* Semester 1 5782
* Code based on code CS 162 assignment from UC Berkeley
*********************/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))
#define MAX 4096

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);


/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "Show the help menu"},
  {cmd_exit, "exit", "Exit command shell"},
  {cmd_pwd,"pwd","current directory working"},
  {cmd_cd,"cd","change directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}
int cmd_pwd(unused struct tokens *tokens) {
  char cwd[1000];
 getcwd(cwd,sizeof(cwd));
printf("%s\n",cwd);
}
int cmd_cd(unused struct tokens *tokens) {
    if (tokens_get_token(tokens,1)==NULL)
    {
    chdir(getenv("HOME"));
    }
    else
    {
    if(chdir(tokens_get_token(tokens,1))!=0)printf("No such file or directory\n");

    }
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

int procces_fork(char *argvs[],char *filename)
{       
   
         
      int new_file;
      char *path = strdup(getenv("PATH"));
      char *sl = "/";
      char colon[] = ":";
      char *direc = strtok(path, colon);
      char *h = malloc(strlen(path) + 1 );

      pid_t ch_pid = fork();
      
      if(ch_pid == 0)
      { 
           
            
           
        execv(argvs[0],argvs);
        for(int i=0; i<strlen(path)+1; i++)   //go over all the path directories to check program
         {
      
            h = strdup(direc);
	        strcat(h, sl);
            strcat(h,argvs[0]);
            if(filename!=NULL) 
            {
            if ((new_file = open(filename, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0)exit(1);
            fflush(stdout);
	        dup2(new_file, 1);
            close(new_file);
            }
           
	        execv(h,argvs);
	        direc = strtok(NULL, colon);
  
         }
         free(h);     
        exit(0);
        
      }
      if(ch_pid > 0)
      {
    
        wait(NULL);
        
          

      }
    
     
return 0;
}
//supportation for pipe_fork function ,return the command that  in index k
static char** CopyArgs(int k, char *argvs[]) 
{
 char **s_argvs = malloc(sizeof(char *) * (4));
    int i=0,j=0,count_bars=0;
    while(argvs[i]!=0)
    {
        if(count_bars==k && strcmp(argvs[i],"|")!=0)
        {
          s_argvs[j]=argvs[i];
          j++;  
        }
        else if(k>count_bars)
        {
        
        }
        else
        {
        break;
        }
        
        if(strcmp(argvs[i],"|")==0)count_bars++;
        i++;
    
    }
    
    s_argvs[j] = 0;
    return s_argvs;
}
// pipe 
int pipe_fork(char *argvs[],int k)
{   

int fd[10][2],i;

char **argv;



for(i=0;i<k;i++)
{

        argv= CopyArgs(i,argvs);//argv take the command in index i for ex : ls -l | grep txt  ->  for i=0 argv=ls -l , for i=1 argv = grep txt.
		
        pipe(fd[i]);
        pid_t pid = fork();
        //child    
		if(pid==0)
        {
			if(i!=k-1){//when the command is not the last one .
				dup2(fd[i][1],1);
				close(fd[i][0]);
				close(fd[i][1]);
			}

			if(i!=0){//when the command is not the first one 
				dup2(fd[i-1][0],0);
				close(fd[i-1][1]);
				close(fd[i-1][0]);
			}
			execvp(argv[0],argv);
			perror("invalid input ");
			exit(1);
		}
		//parent
		if(pid > 0)//second process
        {
			close(fd[i-1][0]);
			close(fd[i-1][1]);
            wait(NULL);
		}
		
}

}





/* Initialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}
void handling(int nu)
{
 
  printf("\n");
  
}

int main(unused int argc, unused char *argv[]) {
  init_shell();
  char *newfile=NULL;
  static char line[4096];
  int line_num = 0;
  
  pid_t shell_child_pid = 0 ;  

  //char *signal_number[] = {"SIGINT", "SIGQUIT", "SIGKILL", "SIGTERM", "SIGTSTP", "SIGCONT", "SIGTTIN", "SIGTTOU"};    //????????
  int signal_number[8] = {2, 3, 9, 15, 20, 18, 21, 22};     //signals number list from "kill -l"
  int n = 0;
  //printf("the str = %ld\n",strlen(signal_number));
  signal(signal_number[n],handling);
  if(shell_child_pid != 0)
      kill(shell_child_pid,signal_number[n]);




  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE the following line to run commands as programs. */
      char *argvs[MAX] = {NULL}; // arrange arguments list
      int i = 0;
      int j = 0;
      int bar =0;
      int count_bars=0;
	  while ( i < tokens_get_length(tokens))
      {
        if(tokens_get_token(tokens, i)!=0)
        {   
        if (strcmp(tokens_get_token(tokens, i),"|")==0)count_bars++;
        if (strcmp(tokens_get_token(tokens, i),">")==0)
        {
        newfile=tokens_get_token(tokens, i+1);
        i+=2;
        }
        else if(strcmp(tokens_get_token(tokens, i),"<")==0)
        {
       
        i++;
        }
        else  
        {  
        argvs[j] = tokens_get_token(tokens, i);
        i++;
        j++;
        }
        }
      }

      if(count_bars==0)//the token is not included a "|" 
          procces_fork(argvs,newfile);
      else  
          pipe_fork(argvs,(count_bars+1));
      newfile=NULL;     

    
    }

    if (shell_is_interactive)
      /* Only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up the tokens from memory */
    tokens_destroy(tokens);
  }

  return 0;
}
// 5782
