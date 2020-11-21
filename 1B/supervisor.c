#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "generator.h"
#include <string.h>

static char *pgm_name;

solution *cBuff;
sem_t *used_sem;
sem_t *free_sem;

static void initializeSemaphores(void)
{
    if ((used_sem = sem_open(SEM_1, O_CREAT | O_EXCL, 0600, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s: used_sem semaphore could not be initialized: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((free_sem = sem_open(SEM_2, O_CREAT | O_EXCL, 0600, MAX_BUFF_SIZE)) == SEM_FAILED)
    {
        fprintf(stderr, "%s: free_sem semaphore could not be initialized: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void setUpSharedMemory(void)
{
    int shmfd;
    if ((shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600)) == -1)
    {
        fprintf(stderr, "%s: shared memory could not be opened: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shmfd, sizeof(cBuff)) == -1)
    {
        fprintf(stderr, "%s: shared memory could not truncated: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((cBuff = mmap(NULL, sizeof(solution[MAX_BUFF_SIZE]), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "%s: shared memory could not be mapped: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int rd_pos = 0;
int currentBestSolutionSize = 9;
static int readFromBuffer(void)
{
    printf("Searching for a better solution....\n");

    if ((sem_wait(used_sem) == -1))
    {
        fprintf(stderr, "%s: used_sem semaphore could not be decremented: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    solution val = cBuff[rd_pos];
    if ((sem_post(free_sem)) == -1)
    {
        fprintf(stderr, "%s: free_sem semaphore could not be incremented: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    rd_pos += 1;
    rd_pos %= MAX_BUFF_SIZE;

    if (val.solutionSize != 0)
    {
        if (val.solutionSize < currentBestSolutionSize)
        {
            currentBestSolutionSize = val.solutionSize;
            printf("Solution with size %d:\n", currentBestSolutionSize);
            for (int i = 0; i < val.solutionSize; i++)
            {
                printf("Edge %ld-%ld needs to be removed.\n", val.edges[i].v1.id, val.edges[i].v2.id);
            }
        }
    }
    else
    {
        printf("Graph Colouring found.\n");
    }

    return val.solutionSize;
}

int main(int argc, char *argv[])
{
    pgm_name = argv[0];

    setUpSharedMemory();

    initializeSemaphores();

    int lookingForSolution = 1;
    while (lookingForSolution)
    {
        lookingForSolution = readFromBuffer();
    }

    return 0;
}

// if ((close(shmfd)) == -1)
// {
//     fprintf(stderr, "%s: shared memory file descriptor could not be closed: %s\n", pgm_name, strerror(errno));
//     exit(EXIT_FAILURE);
// }