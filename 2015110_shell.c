#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include <stdio.h>

static struct termios old, new;
const int WRITE_END = 1;
const int READ_END = 0;

/* Initialize new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  new = old; /* make new settings same as old settings */
  new.c_lflag &= ~ICANON; /* disable buffered i/o */
  new.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}

int inpSize ;

char **split(char **inp,char *line)
{
	// printf("line : %s\n",line);
	inp = malloc(1024*sizeof(char*));
	int i=0;
	char *tok = malloc(1024*sizeof(char*));
	tok = strtok(line," \t\r\n\a");
	while(tok != NULL)
	{
		inp[i++] = tok;
		tok = strtok(NULL," \t\r\n\a");
		// printf("%s\n",inp[i-1]);
	}
	inp[i] = NULL;
	inpSize = i;
	return inp;
}

char *IGNORE_QUOTES(char *line)
{
	char *str;
	str = malloc(1000);
	int sidx=0;
	int i;
	for(i=0;i<strlen(line);i++)
	{
		if(line[i] == '\'')
			continue;
		// printf("%c",line[i]);
		str[sidx++] = line[i];
	}
	// printf("\n");
	str[sidx] = '\0';
	return str;
}

char *PARSE(char *line)
{
	int x;
	for(x=0;x<strlen(line);x++)
	{
		if(line[x] == ' ')
			break;
	}
	line += x+1;
	while(*line == ' ')
		line++;
	int i;
	bool is_quotes = false;
	for(i=x+1;i<strlen(line);i++)
	{
		if(line[i] == '\'')
			is_quotes = !is_quotes;
		if(line[i] == ' ' && is_quotes == false)
		{
			line[i] = '\0';
			line = IGNORE_QUOTES(line);
			return line;
		}
	}
	line = IGNORE_QUOTES(line);
	return line;
}

void My_CD(char *line)
{
	line = PARSE(line);
	// printf("%s\n",line);

	if(chdir(line) != 0)
	{
		fprintf(stderr, "No such file or directory exists\n");
	}
}

bool X = false;
void RUN_CD(char **inp)
{



	if (inp[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
  	if(X)
  		strcpy(inp[1],"Desktop/My Songs");
  	X=true;
  	// printf("%s\n",inp[1]);
    if (chdir(inp[1]) != 0) {
      perror("lsh");
    }
  }
  return ;
}

int is_in=0;
int is_out=0;
char in_fname[100];
char out_fname[100];

int execute(char **inp,int input, int is_first, int is_last)
{
	// printf("%s %d %d %d\n",inp[0],input, is_first , is_last);
	int fd[2];
	pipe(fd);
	pid_t fpid,wpid;
	int status;
	fpid = fork();
	if(fpid == 0)
	{
		if(is_last == 1)
		{
			if(is_out)
			{
				status = creat(out_fname,0644);
				dup2(status,STDOUT_FILENO);
				is_out = 0;
			}
			if(is_in)
			{
				status = open(in_fname,O_RDONLY,0);
				dup2(status, STDIN_FILENO);
				is_in = 0;
			}
			dup2(input,STDIN_FILENO);
		}
		else if(is_first == 1)
			dup2( fd[WRITE_END],STDOUT_FILENO);
		else
		{
			dup2( input , STDIN_FILENO);
			dup2( fd[WRITE_END],STDOUT_FILENO);
		}

		if(execvp(inp[0],inp)==-1)
		{
			printf("syntax error!\n");
			exit(EXIT_FAILURE);
		}
	}
	// else if(fpid > 0)
	// {
	// 	do
	// 	{
	// 		wpid = waitpid(fpid,&status,WUNTRACED);
	// 	}while(!WIFEXITED(status) && !WIFSIGNALED(status));
	// }

	if (input)
		close(input);

	close(fd[WRITE_END]);

	if(is_last)
		close(fd[READ_END]);
	return fd[READ_END];

}

char prevL[1000],line[1000];
int i,prev_i;
int x = 0;

char history[1000][100];
char M_history[1000][100];
int hidx=-1;

void FUNC(int x)
{
}


void HISTORY_HANDLER()
{
	int j=0;
	for(j=0;j<hidx;j++)	
		printf(" %d %s\n",j+1,M_history[j]);
}

void HELP()
{
	printf(" cd\n");
	printf(" history\n");
	printf(" help\n");
	printf(" exit\n");
}

bool CORNER_CASES(char line[])
{
	char **inp;
	char copy[1000];
	strcpy(copy,line);
	inp = split(inp,copy);
	// printf("%s\n",inp[0]);
	if(strcmp(inp[0],"exit") == 0)
	{
		exit(0);
		return true;
	}
	if(strcmp(inp[0],"history") == 0)
	{
		HISTORY_HANDLER();
		return true;
	}
	if(strcmp(inp[0],"cd") == 0)
	{
		My_CD(line);
		return true;
	}
	if(strcmp(inp[0],"help") == 0)
	{
		HELP();
		return true;
	}
	return false;

}

char sp[1000][1000];

void INIT()
{
	int i;
	for(i=0;i<1000;i++)
	{
		int j;
		for(j=0;j<i;j++)
		{
			sp[i][j] = ' ';
		}
		sp[i][i] = '\0';
	}
}


int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}


int main()
{

	INIT();

	signal(SIGINT,FUNC);
	char **inp;
	char c;
	i=0;
	hidx = 0;
	while(1)
	{
		for(i=0;i<hidx;i++)
			strcpy(history[i],M_history[i]);
		// printf("%d\n",hidx);
		// for(i=0;i<hidx;i++)
		// 	printf("%s\n",history[i]);
			
		char curr_dir[1001];
		getcwd(curr_dir,sizeof(curr_dir));
			
		i=0;
		int hidx_query = hidx;
		printf("\rUtsav-Laptop:~%s$ ",curr_dir);
		int len = 19 + strlen(curr_dir);
		fflush(stdout);
		while((c = getch()) != '\n')
		{
			if(c == 27)
			{
				c = getch();
				c = getch();
				if(c == 65 && hidx_query)
				{
					printf("\r%s",sp[i+1+len]);
					strcpy(line,history[--hidx_query]);
					i = strlen(line);
					int x;
					printf("\rUtsav-Laptop:~%s$ ",curr_dir);
					printf("%s",line);
					fflush(stdout);
				}
				if(c == 66)
				{
					printf("\r%s",sp[i+1+len]);
					strcpy(line,history[++hidx_query]);
					i = strlen(line);
					// printf("\r                                                                                                                                                           ");
					// printf("\r                                                                                                    ");
					printf("\rUtsav-Laptop:~%s$ ",curr_dir);
					printf("%s",line);
					// if(hidx_query == hidx)
					// 	printf("%s %d\n",line, hidx_query);
					fflush(stdout);
				}
				// printf("%d %d\n",hidx_query,hidx);
			}
			else if(c == 127)
			{
				if(i == 0)
					continue;
				line[--i] = '\0';
				// printf("\r");
				printf("\r%s",sp[i+1+len]);
				printf("\rUtsav-Laptop:~%s$ ",curr_dir);
				printf("%s",line);
				line[i] = '\0';
				strcpy(history[hidx_query] , line);
			}
			else
			{
				printf("%c",c);
				fflush(stdout);
				line[i++] = c;
				line[i] = '\0';
				strcpy(history[hidx_query] , line);
			}
			// printf("%s\n",history[hidx]);
		}
		printf("\n");
		if(i == 0)
			continue;
		line[i] = '\0';



		strcpy(M_history[hidx],line);
		hidx++;
		is_out=0;
		is_in=0;

		int j;
		int L = i+1;

		bool inend=true;
		int oidx=0,iidx=0;
		for(j=0;j<i;j++)
		{
			if(line[j] == '>')
			{
				is_out = 1;
				L=j;
				line[L] = '\0';
				int k;
				j++;
				bool isq = false;
				while(line[j] == ' ')
					j++;
				if(line[j] == '\'')
				{
					isq = true;
					j++;
				}
				for(k=j;k<i;k++)
				{
					if(line[k] == '<' || (line[k] == ' ' && isq == false))
					{
						out_fname[oidx] = '\0';
						break;
					}
					if(k == i-1)
					{
						if(line[k] == '\'')
							break;	
					}
					out_fname[oidx++] = line[k];
				}
			}
			if(line[j] == '<')
			{
				is_in = 1;
				L=j;
				line[L] = '\0';
				int k;
				j++;
				bool isq = false;
				while(line[j] == ' ')
					j++;
				if(line[j] == '\'')
				{
					isq = true;
					j++;
				}
				for(k=j;k<i;k++)
				{
					if(line[k] == '>' || (line[k] == ' ' && isq == false))
					{
						in_fname[iidx] = '\0';
						break;
					}
					if(k == i-1)
					{
						if(line[k] == '\'')
							break;
					}
					in_fname[iidx++] = line[k];
				}
			}
		}
		out_fname[oidx] = '\0';
		in_fname[iidx] = '\0';
		// printf("is_in :%d\n",is_in);
		// printf("is_out :%d\n",is_out);
		// printf("outfile :%s\n",out_fname);
		// printf("in file :%s\n",in_fname);

		if(is_in == 1 && !is_regular_file(in_fname))
		{
			printf("No file named : %s\n",in_fname);
			continue;
		}

		int offset=0;
		int is_first = 1;
		int is_last = 0;
		int input = 0;
		x = 0;
		prev_i = i;

		int ct = 0;
		bool x = CORNER_CASES(line);
		// printf("HERE : %s\n",line);
		if(!x)
		{
			// printf("231231\n");
			for(j=0;j<i;j++)
			{
				if(line[j] == '|')
				{
					// printf("%s\n",line);
					line[j] = '\0';
					inp = split(inp,line+offset);
					// printf("in main : %s\n",line+offset);
					offset = j+1;
					input = execute(inp, input , is_first , is_last);
					ct++;
					// printf("YO\n");
					is_first = 0;
				}
			}
			// for(j=offset;j<10;j++)
			// 	printf("%s",line[j]);
			// printf("  chutad \n");
			// printf("in main : %s\n",line+offset);
			inp = split(inp, line+offset);
			input = execute(inp, input , is_first , 1);
			ct++;
			fflush(stdout);
		}

		for(j=0;j<ct;j++)
			wait(NULL);

	}
}