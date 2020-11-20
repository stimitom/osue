#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "generator.h"

#define SHM_NAME "/genshm"

static char *pgm_name;

solution *cBuff[10];

int rd_pos = 0;
static solution read()
{
    // sem_wait(used_sem);
    solution val = *cBuff[rd_pos];
    // sem_post(free_sem);
    rd_pos += 1;
    rd_pos %= sizeof(cBuff);
    return val;
}

int main(int argc, char *argv[])
{
    pgm_name = argv[0];

    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);

    if (shmfd == -1)
    {
        fprintf(stderr, "%s: shared memory could not be opened: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shmfd, sizeof(cBuff)) == -1)
    {
        fprintf(stderr, "%s: shared memory could not truncated: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    solution *buffer[10];
    *buffer = mmap(NULL, sizeof(*buffer), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    if (buffer == MAP_FAILED)
    {
        fprintf(stderr, "%s: shared memory could not be mapped: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((close(shmfd)) == -1)
    {
        fprintf(stderr, "%s: shared memory file descriptor could not be closed: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}