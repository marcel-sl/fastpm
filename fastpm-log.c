#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <mpi.h>

#include "fastpm-log.h"


static fastpm_msg_handler msg_handler = NULL;
static void * msg_handler_userdata = NULL;
static MPI_Comm msg_handler_comm = (MPI_Comm) NULL;

void fastpm_default_msg_handler(
            const enum FastPMLogLevel level,
            const enum FastPMLogType type,
            const int errcode,
             const char * message, 
            MPI_Comm comm,
            void * userdata) {
    int ThisTask;
    MPI_Comm_rank(comm, &ThisTask);
    if(type == COLLECTIVE) {
        MPI_Barrier(comm);
    }
    if(ThisTask != 0 && type == COLLECTIVE)
        return;
    fprintf(stdout, "%s", message);
    fflush(stdout);

    if(level == ERROR) abort();
}

void fastpm_set_msg_handler(fastpm_msg_handler handler, MPI_Comm comm, void * userdata)
{
    msg_handler = handler;
    msg_handler_userdata = userdata;
    msg_handler_comm = comm;
}

static double now()
{
    return MPI_Wtime();
}

static char * process(const char * fmt) {
    char buf[128];

    static double t0 = -1.0;
    if(t0 < 0) t0 = now();
    sprintf(buf, "[ %012.04f ]: ", now() - t0);
    char * ret = malloc(strlen(fmt) + strlen(buf) + 1);
    strcpy(ret, buf);
    strcat(ret, fmt);
    return ret;
}


void fastpm_log2(const enum FastPMLogLevel level, 
            const enum FastPMLogType type,
            const int code, 
            const char * fmt, va_list argp) {
    char buffer[4096];
    char * newfmt = process(fmt);

    if (msg_handler == NULL) {
        fastpm_set_msg_handler(fastpm_default_msg_handler, MPI_COMM_WORLD, NULL);
    }

    vsprintf(buffer, newfmt, argp);
    msg_handler(level, type, code, buffer, msg_handler_comm, msg_handler_userdata);

    free(newfmt);
}

void fastpm_ilog(const enum FastPMLogLevel level, 
            const int code, 
            const char * fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    fastpm_log2(level, INDIVIDUAL, code, fmt, argp); 
    va_end(argp);
}

void fastpm_log(const enum FastPMLogLevel level, 
            const int code, 
            const char * fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    fastpm_log2(level, COLLECTIVE, code, fmt, argp); 
    va_end(argp);
}

void fastpm_info(const char * fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    fastpm_log2(INFO, COLLECTIVE, 0, fmt, argp); 
    va_end(argp);
}
void fastpm_raise(const int code, const char * fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    fastpm_log2(ERROR, INDIVIDUAL, code, fmt, argp); 
    va_end(argp);
}