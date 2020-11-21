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
#include <unistd.h>


static char *pgm_name;

circBuff *cBuff;
sem_t *used_sem;
sem_t *free_sem;
sem_t *mutex_sem;
int shmfd;

int rd_pos = 0;
int currentBestSolutionSize = 9;
static int readFromBuffer(void)
{

    if ((sem_wait(used_sem) == -1))
    {
        fprintf(stderr, "%s: used_sem semaphore could not be decremented: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    solution val = cBuff->array[rd_pos];
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
            printf("Searching for a better solution....\n");
        }
    }
    else
    {
        printf("Graph Colouring found.\n");
    }

    return val.solutionSize;
}

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

    if ((mutex_sem = sem_open(SEM_3, O_CREAT | O_EXCL, 0600, 1)) == SEM_FAILED)
    {
        fprintf(stderr, "%s: mutex_sem semaphore could not be initialized: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void setUpSharedMemory(void)
{

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

    if ((cBuff = mmap(NULL, sizeof(cBuff), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "%s: shared memory could not be mapped: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void cleanUp(void)
{

    if (munmap(cBuff, sizeof(cBuff)) == -1)
    {
        fprintf(stderr, "%s: shared memory file descriptor could not be unmapped: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (shm_unlink(SHM_NAME) == -1)
    {
        fprintf(stderr, "%s: shared memory file descriptor could not be unlinked: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((close(shmfd)) == -1)
    {
        fprintf(stderr, "%s: shared memory file descriptor could not be closed: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    sem_close(free_sem);
    sem_close(used_sem);
    sem_close(mutex_sem); 
    sem_unlink(SEM_1); 
    sem_unlink(SEM_2);
    sem_unlink(SEM_3);
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

    cleanUp();
    return 0;
}
