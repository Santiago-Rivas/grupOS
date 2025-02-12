#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdio.h>
#include <sys/wait.h>
#include "../../lib/lib.h"

#define WORKERS_MAX 6

#define WORKER_PATH "./bin/worker"
#define WORKER_NAME "worker"

#define INITIAL_LOAD 2

#define OUTPUT_PATH "output.txt"

#define SUCCESS 0
#define PARAMETER_ERROR 1
#define MEMORY_ERROR 2
#define SELECT_ERROR 3
#define RW_ERROR 4
#define FILE_ERROR 5

// Worker struct which contains its pipes & a file to read from
typedef struct Worker
{
    int pid;
    int pipe_write;
    int pipe_read;
    FILE *file_read;
} Worker;

void close_pipes(int pipe_fds[][2], size_t count);
void close_files(FILE *files[], size_t count);

// Opens pipes, forks workers, opens files & writes data. Checks for errors at each step
int workers_spawn(Worker workers[], size_t count, fd_set *read_workers);

// Closes pipes & waits for the workers to finish
int workers_free(Worker workers[], size_t count);

#endif // APPLICATION_H
