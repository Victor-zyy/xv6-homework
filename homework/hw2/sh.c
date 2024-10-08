#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be uun (e.g., an execcmd)
  char *file;        // the input/output file
  int flags;         // flags for open() indicating read or write
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

struct listcmd {
	int type;					 // ';'
	struct cmd *left;  // sequential cmds ready to be exectuted
	struct cmd *right;  // sequential cmds ready to be exectuted
};

struct backcmd {
	int type; 				// &
	struct cmd *subcmd;
};

int fork1(void);  // Fork but exits on failure.
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;
  struct listcmd  *lcmd;
  struct backcmd  *bcmd;

  if(cmd == 0)
    _exit(0);
  
  switch(cmd->type){
  default:
    fprintf(stderr, "unknown runcmd\n");
    _exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      _exit(0);
    // Your code here ...
		//execv(ecmd->argv[0], ecmd->argv);
#if 1
		if(execvp(ecmd->argv[0], ecmd->argv) == -1){
    	fprintf(stderr, "exec error please check errno to find what types of error\n");
		}
#endif
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    //fprintf(stderr, "redir not implemented\n");
    // Your code here ...
		if(close(rcmd->fd) == -1){
    	fprintf(stderr, "close file descriptor error please check errno to find what types of error\n");
		}
		rcmd->fd = open(rcmd->file, rcmd->flags, S_IRUSR | S_IWUSR);
		if(r == -1){
    	fprintf(stderr, "open file error please check errno to find what types of error\n");
		}
    runcmd(rcmd->cmd);
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    //fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
		// stdin 0 stdout 1 stderr 2
		// pipe[0] for read pipe[1] for write
		if(pipe(p) == -1){
    	fprintf(stderr, "pipe error please check errno to find what types of error\n");
		}

		if(fork1() == 0){	//child process
			if(dup2(p[1],1) == -1){
    		fprintf(stderr, "dup2(1) error please check errno to find what types of error\n");
			}
			if(close(p[0]) == -1){
    		fprintf(stderr, "close error please check errno to find what types of error\n");
			}
			runcmd(pcmd->left);

		}

		wait(NULL);	//wait for child

		if(close(p[1]) == -1){
   		fprintf(stderr, "close error please check errno to find what types of error\n");
		}
		if(dup2(p[0], 0) == -1){
    	fprintf(stderr, "dup2(0) error please check errno to find what types of error\n");
		}

		/*
		if(close(p[0]) == -1){
    	fprintf(stderr, "close(0) error please check errno to find what types of error\n");
		}
		*/
		runcmd(pcmd->right);
    break;

	case ';':
    lcmd = (struct listcmd*)cmd;
		//
		if(fork1() == 0){
			runcmd(lcmd->left);
		}
		wait(NULL);
		runcmd(lcmd->right);
		break;

	case '&':
    bcmd = (struct backcmd*)cmd;
		if(fork1() == 0)
				runcmd(bcmd->subcmd);
		break;
  }    
  _exit(0);
}

int
getcmd(char *buf, int nbuf)
{
  if (isatty(fileno(stdin)))
    fprintf(stdout, "6.828$ ");
  memset(buf, 0, nbuf);
  if(fgets(buf, nbuf, stdin) == 0)
    return -1; // EOF
  return 0;
}

int
main(void)
{
  static char buf[100];
  int fd, r;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf)); //child process
    wait(&r);
  }
  exit(0);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->flags = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ';';
  cmd->left = left;
  cmd->right = right;

  return (struct cmd*)cmd;
}


struct cmd*
backcmd(struct cmd *subcmd)
{
	struct backcmd *cmd;
	cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '&';
  cmd->subcmd = subcmd;

  return (struct cmd*)cmd;
}	
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>;()&";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
	case ';':
	case '&':
	case '(':
	case ')':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

//char string like: echo "6.828 is cool" > x.txt
//
int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
	//eliminate the first whitespace of the command like "command" -> "   command"
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
	
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *parselist(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;		// points to end of string
  struct cmd *cmd;

  es = s + strlen(s);
	// parseline to parse
  cmd = parseline(&s, es);
	//final check
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

// echo "my friends" > t.txt & ls 
// such command like " ls | wc
// echo "my friends" > t.txt | ls 
//
// hierarchy
// 1.parse pipe
// 2.parse exec
// 3.parse redirection

// parse command
// 1.list
// 2.subcommand
// 3.pipe
// 4.exec
// 5.redirect
//
struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
	while(peek(ps, es, "&")){
		gettoken(ps, es, 0, 0);
		cmd = backcmd(cmd);
	}
	if(peek(ps, es, ";")){
		gettoken(ps, es, 0, 0);
		cmd = listcmd(cmd, parseline(ps, es));
	}
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es)); // recrusive call back
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
	struct cmd *cmd;
	if(!peek(ps, es, "(")){
      fprintf(stderr, "parseblock error\n");
      exit(-1);
	}

	gettoken(ps, es, 0, 0);

	cmd = parseline(ps, es);
	if(!peek(ps, es, ")")){
      fprintf(stderr, "syntax error missing )\n");
      exit(-1);
	}
	gettoken(ps, es, 0, 0);
	cmd = parseredirs(cmd, ps, es);

	return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
	if(peek(ps, es, "(")){
			return parseblock(ps, es);
	}
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
