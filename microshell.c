#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct	 s_cmd
{
	char		 **args;
	int			 pipe;
	struct s_cmd *next;
}				 t_cmd;

int ft_strlen(char *s)
{
	int i = 0;

	while (s && *s++)
		i++;
	return (i);
}

int error(int fatal, char *str, char *subj)
{
	if (fatal)
		return(error(0, "error: fatal", NULL));
	write(2, str, ft_strlen(str));
	write(2, subj, ft_strlen(subj));
	write(2, "\n", 1);
	return (1);
}

int	count_cmd(char **argv, int i)
{
	while (argv[i] && *argv[i])
	{
		if (!strcmp(argv[i], ";") || !strcmp(argv[i], "|"))
			break;
		i++;
	}
	return (i);
}

t_cmd *cmd_new(char **args, int pipe)
{
	t_cmd *result;

	if (!(result = malloc(sizeof(t_cmd))))
		return (0);
	result->args = args;
	result->pipe = pipe;
	result->next = NULL;
	return (result);
}

void	cmdaddback(t_cmd **cmd, t_cmd *new)
{
	if (!new)
		return ;
	if (!*cmd)
		*cmd = new;
	else
	{
		while ((*cmd)->next)
			*cmd = (*cmd)->next;
		(*cmd)->next = new;
	}
}

t_cmd *parser(int argc, char **argv, int i)
{
	t_cmd	*cmd;
	char	**args;
	int		len = 0;
	int		k = 0;
	int		pipe;

	if (++i >= argc)
		return (NULL);
	len = count_cmd(argv, i) - i;
	if (!(args = malloc(sizeof(char *) * len + 1)))
		return (0);
	while (len--)
		args[k++] = argv[i++];
	args[k] = NULL;
	pipe = (argv[i] && !strcmp(argv[i], "|")) ? 1 : 0;
	cmd = cmd_new(args, pipe);
	cmdaddback(&cmd, parser(argc, argv, i));
	if (cmd->next == NULL)
		return (cmd);
	return (cmd);
}

void free_cmd(t_cmd **cmd)
{
	while(*cmd)
	{
		free((*cmd)->args);
		free(*cmd);
		cmd++;
	}
}

int cd(char **args)
{
	if (!*(args + 1) || *(args + 2))
		return (error(0, "error: cd: bad arguments", NULL));
	if (chdir(args[1]) < 0)
		return (error(0, "error: cd: cannot change directory to ", args[1]));
	return 0;
}

int bin(char **args, char **envp)
{
	pid_t child;
	int status = 0;

	child = fork();
	if (child < 0)
		return (error(1, 0,0));
	else if (child == 0)
	{
		if ((status = execve(args[0], args, envp)) == -1)
			return(error(0, "error: cannot execute ", args[0]));
		exit(status);
	}
	else
		waitpid(child, &status, 0);
	return (WEXITSTATUS(status));
}

int pipe_fork(char **args, char **envp)
{
	pid_t	child;
	int		fd_p[2];
	int		status = 0;

	if (pipe(fd_p) == -1)
		error(1, 0,0);
	child = fork();
	if (child == 0)
	{
		close(fd_p[0]);
		close(1);
		dup2(fd_p[1], 1);
		status = bin(args, envp);
		close(fd_p[1]);
		exit(status);
	}
	else
	{
		close(fd_p[1]);
		waitpid(child, &status, 0);
		close(0);
		dup2(fd_p[0], 0);
		close(fd_p[0]);
	}
	return (WEXITSTATUS(status));
}

int main(int argc, char **argv, char **envp)
{
	t_cmd	*cmd;
	t_cmd	*head;
	int fd_in;
	int status = 0;

	fd_in = dup(0);
	cmd = parser(argc, argv, 0);
	head = cmd;
	while (cmd && cmd->args[0])
	{
		if (!strcmp(cmd->args[0], "cd"))
			status = cd(cmd->args);
		else
		{
			if (cmd->pipe)
				status = pipe_fork(cmd->args, envp);
			else
			{
				status = bin(cmd->args, envp);
				dup2(fd_in, 0);
			}

		}
		cmd = cmd->next;
	}
	close(fd_in);
	//free_cmd(&head);
	while(1)
		sleep(1);
	return (status);
}
