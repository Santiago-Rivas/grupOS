// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "application.h"

void close_pipes(int pipe_fds[][2], size_t count)
{
    size_t i;
    for (i = 0; i < count; i++)
    {
        close_pipe(pipe_fds[i]);
    }
}

void close_files(FILE *files[], size_t count)
{
    size_t i;
    for (i = 0; i < count; i++)
    {
        fclose(files[i]);
    }
}

int workers_spawn(Worker workers[], size_t count, fd_set *read_workers)
{
    if (count > WORKERS_MAX)
    {
        return -1;
    }

    // open pipes
    int pipes_path[WORKERS_MAX][2];
    int pipes_hash[WORKERS_MAX][2];
    size_t i;
    for (i = 0; i < count; i++)
    {
        if (pipe(pipes_path[i]) == -1)
        {
            close_pipes(pipes_path, i);
            close_pipes(pipes_hash, i);
            return -1;
        }
        if (pipe(pipes_hash[i]) == -1)
        {
            close_pipes(pipes_path, i + 1);
            close_pipes(pipes_hash, i);
            return -1;
        }
    }

    // fork workers
    int cpid[WORKERS_MAX];
    for (i = 0; i < count; i++)
    {
        cpid[i] = fork();
        if (cpid[i] == -1)
        {
            close_pipes(pipes_path, count);
            close_pipes(pipes_hash, count);
            return -1;
        }

        // child code
        if (cpid[i] == 0)
        {
            if (dup2(pipes_path[i][READ_END], STDIN_FILENO) == -1 || dup2(pipes_hash[i][WRITE_END], STDOUT_FILENO) == -1)
            {
                _exit(1);
            }
            close_pipes(pipes_path, count);
            close_pipes(pipes_hash, count);
            char *const argv[2] = {WORKER_NAME, NULL};
            execv(WORKER_PATH, argv);
            perror("EXECV ERROR");
            _exit(1);
        }
    }

    // open files
    FILE *files_hash[WORKERS_MAX];
    for (i = 0; i < count; i++)
    {
        files_hash[i] = fdopen(pipes_hash[i][READ_END], "r");
        if (files_hash[i] == NULL)
        {
            close_files(files_hash, i);
            close_pipes(pipes_path, count);
            close_pipes(pipes_hash + i, count - i);
            return -1;
        }
        // Unbuffered stream
        setbuf(files_hash[i], NULL);
    }

    // success: write data
    FD_ZERO(read_workers);
    for (i = 0; i < count; i++)
    {
        close(pipes_path[i][READ_END]);
        close(pipes_hash[i][WRITE_END]);
        workers[i].pid = cpid[i];
        workers[i].pipe_write = pipes_path[i][WRITE_END];
        workers[i].pipe_read = pipes_hash[i][READ_END];
        workers[i].file_read = files_hash[i];
        FD_SET(workers[i].pipe_read, read_workers);
    }

    return 0;
}

int workers_free(Worker workers[], size_t count)
{
    if (count > WORKERS_MAX)
    {
        return -1;
    }

    // close pipes
    size_t i;
    for (i = 0; i < count; i++)
    {
        close(workers[i].pipe_write);
        fclose(workers[i].file_read);
    }

    // wait for workers to finish
    for (i = 0; i < count; i++)
    {
        waitpid(workers[i].pid, NULL, 0);
    }

    return 0;
}
