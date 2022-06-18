#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<fcntl.h>

#define MAX_CMD_NUM 20
#define MAX_CMD_LEN 50

#define NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define BLUE         "\033[0;32;34m"

void run_command(char *line,int indes,int outdes);
void parse_command(char *line);
int is_file_name(char c);
int is_blank(char c);
void print_error(char *info);
int main()
{
	char pwd[100];
	char line[100];
	size_t pwd_size = 100;
	size_t line_size = 100;
	
	memset(pwd,0,pwd_size);
	memset(line,0,line_size);
	
	printf(LIGHT_RED"Welcome to SYQ\'s shell!\n"NONE);
	
	while(1)
	{
		printf(GREEN"syq@mayousyq"NONE":"BLUE"%s"NONE"$ ",getcwd(pwd,pwd_size));
		fgets(line,line_size,stdin);
		line[strlen(line)-1] = 0;
		//printf("line:%s\n",line);
		parse_command(line);
	}
	
	return 0;
}
void run_command(char *line,int indes,int outdes)
{
	pid_t pid;
	pid_t pid_return;
	
	//printf(GREEN"In function run_command"NONE);
	//printf("\n");
	
	if((pid=fork()) ==0)
	{
		dup2(indes,0);
		dup2(outdes,1);
		execl(line,"",NULL);
	}
	else if(pid>0)
	{
   		pid_return = wait(NULL);
   		//printf(GREEN"pid_return:%d\n"NONE,pid_return);
   		if(pid_return < 0)
   		{
   			print_error("Failed to run the program");
   		}
	}
	else
	{
   		print_error("Fork failed");
	}
	
	//printf(GREEN"At the end of function run_command\n"NONE);
}
void parse_command(char *line)
{
	int i = 0;
	int len = strlen(line);
	char cmds[MAX_CMD_NUM][MAX_CMD_LEN];
	int descriptors[MAX_CMD_NUM][2];
	int input_descriptor = STDIN_FILENO;
	int input_mode = 0;
	int output_descriptor = STDOUT_FILENO;
	int output_mode = 0;
	int cmd_counter = 0;
	int cmd_str_counter = 0;
	char name_recorder[MAX_CMD_LEN];
	int name_counter = 0;
	int pipes[MAX_CMD_NUM][2];
	
	//printf(GREEN"In function parse_command\n"NONE);
	
	while(i<len)
	{
		if(is_file_name(line[i]))
		{
			//printf(GREEN"line[i] is filename\n"NONE);
			while(is_file_name(line[i]) && i<len)
			{
				cmds[cmd_counter][cmd_str_counter++] = line[i++];
				if(cmd_str_counter > MAX_CMD_LEN)
				{
					print_error("cmd length out of bound");
				}
			}
			cmds[cmd_counter][cmd_str_counter] = 0;
			cmd_counter++;
			if(cmd_counter > MAX_CMD_NUM)
			{
				print_error("cmd number out of bound");
			}
			cmd_str_counter = 0;
		}
		else if(line[i] == '<')
		{
			//printf(GREEN"line[i] is \"<\"\n"NONE);
			if(input_mode)
			{
				print_error("Multiple input redirections");
			}
			i++;
			while(is_blank(line[i]))
			{
				i++;
			}
			while(is_file_name(line[i]) && i<len)
			{
				name_recorder[name_counter++] = line[i++];
				if(cmd_str_counter > MAX_CMD_LEN)
				{
					print_error("cmd length out of bound");
				}
			}
			name_recorder[name_counter] = 0;
			name_counter = 0;
			input_descriptor = open(name_recorder,O_RDONLY);
			if(input_descriptor < 0)
			{
				print_error("File does not exist");
			}
		}
		else if(line[i] == '>')
		{
			//printf(GREEN"line[i] is \">\"\n"NONE);
			if(output_mode)
			{
				print_error("Multiple output redirections");
			}
			i++;
			if(line[i] == '>')
			{
				i++;
				output_mode = 2;
				//printf(GREEN"mode 2\n"NONE);
			}
			else
			{
				output_mode = 1;
				//printf(GREEN"mode 1\n"NONE);
			}
			while(is_blank(line[i]))
			{
				i++;
			}
			while(is_file_name(line[i]) && i<len)
			{
				name_recorder[name_counter++] = line[i++];
				if(cmd_str_counter > MAX_CMD_LEN)
				{
					print_error("cmd length out of bound");
				}
			}
			name_recorder[name_counter] = 0;
			name_counter = 0;
			if(output_mode == 1)
			{
				output_descriptor = open(name_recorder,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU|S_IRWXG|S_IRWXO);
			}
			else
			{
				output_descriptor = open(name_recorder,O_WRONLY|O_CREAT|O_APPEND,S_IRWXU|S_IRWXG|S_IRWXO);
			}
			if(output_descriptor < 0)
			{
				print_error("Failed to open file");
			}
		}
		else if(line[i] == '|')
		{
			//printf(GREEN"line[i] is \"|\"\n"NONE);
			//printf(GREEN"cmd_counter=%d\n"NONE,cmd_counter);
			pipe(pipes[cmd_counter]);
			descriptors[cmd_counter - 1][1] = pipes[cmd_counter][1];
			descriptors[cmd_counter][0] = pipes[cmd_counter][0];
			i++;
		}
		else if(is_blank(line[i]))
		{
			//printf(GREEN"line[i] is blank\n"NONE);
			i++;
		}
		else
		{
			print_error("Invalid character");
		}
		//printf(GREEN"line[i] parse end\n"NONE);
	}
	
	descriptors[0][0] = input_descriptor;
	descriptors[cmd_counter - 1][1] = output_descriptor;
	
	//printf(GREEN"Parse end, running start\n"NONE);
	
	for(i=0;i<cmd_counter;i++)
	{
		//printf(GREEN"i=%d\n"NONE,i);
		run_command(cmds[i],descriptors[i][0],descriptors[i][1]);
		//printf(GREEN"Out of run command\n"NONE);
		if(descriptors[i][0] != STDIN_FILENO) close(descriptors[i][0]);
		if(descriptors[i][1] != STDOUT_FILENO) close(descriptors[i][1]);
	}
	
	//printf(GREEN"Running end\n"NONE);
}
int is_file_name(char c)
{
	return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' || c == '.' || c == '/';
}
int is_blank(char c)
{
	return c == ' ' || c == '\t';
}
void print_error(char *info)
{
	printf(RED"Error!\n"NONE);
	printf(RED"%s\n"NONE,info);
	exit(1);
}
