#include "shared.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

void setUpSharedMemory(void);
void tearDownSharedMemory(void);
void setUpSemaphores(void);
void tearDownSemaphores(void);
void writeToBuffStack(stackElem elem);
void tearDown(void);
char *pgm_name;
int shm;
buffStack *bStack;
sem_t *usedSem;
sem_t *freeSem;
sem_t *mutexSem;

int main(int argc, char *argv[])
{

    pgm_name = argv[0];

    if (atexit(tearDown) != 0)
    {
        fprintf(stderr, "atexit failed\n");
    }

    setUpSharedMemory(); 
    setUpSemaphores();

    if (argc <= 1)
    {
        fprintf(stderr, "Arguments need to be provided");
        exit(EXIT_FAILURE);
    }
    for (int i = 1; i < argc; i++)
    {
        long int e1;
        char *nptr = argv[i];
        char *endptr;
        e1 = strtol(nptr, &endptr, 10);

        if (endptr != NULL)
        {
            if (endptr == nptr || endptr[0] != '-')
            {
                fprintf(stderr, "%s: input could not be parsed. At least one edge is malformed before the '-' sign.\n", pgm_name);
                exit(EXIT_FAILURE);
            }
        }

        printf(" e1 %ld\n", e1);


        long int e2;
        char *endptr2;
        e2 = strtol(endptr +1, &endptr2, 10);

        if (endptr2 != NULL)
        {
            if (endptr2 == (endptr + 1) || endptr2[0] != '\0')
            {
                fprintf(stderr, "%s: input could not be parsed. At least one edge is malformed after the '-' sign.\n", pgm_name);
                exit(EXIT_FAILURE);
            }
        }

        printf(" e2 %ld\n", e2);

        stackElem elem = {e1, e2};

        writeToBuffStack(elem);
    }
}

void tearDown(void)
{
    tearDownSharedMemory();
    tearDownSemaphores();
}

void writeToBuffStack(stackElem elem)
{
    if (sem_wait(mutexSem) == -1){
        fprintf(stderr, "%s something went wrong with sem wait of %s: %s", pgm_name, SEM3, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (sem_wait(freeSem) == -1){
        fprintf(stderr, "%s something went wrong with sem wait of %s: %s", pgm_name, SEM2, strerror(errno));
        exit(EXIT_FAILURE);
    }
 

    bStack->items[bStack->top++] = elem; 

    if(sem_post(usedSem) == -1){
     fprintf(stderr, "%s something went wrong with sem post of %s: %s", pgm_name, SEM1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(sem_post(mutexSem) == -1){
     fprintf(stderr, "%s something went wrong with sem post of %s: %s", pgm_name, SEM3, strerror(errno));
        exit(EXIT_FAILURE);
    }

}



void setUpSharedMemory(void)
{

    if ((shm = shm_open(SHM_NAME_1, O_RDWR, 0600)) == -1)
    {
        fprintf(stderr, "%s could not be opened: %s", SHM_NAME_1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((bStack = mmap(NULL, sizeof(buffStack), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "%s could not be mapped: %s", SHM_NAME_1, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void tearDownSharedMemory(void)
{

    if (munmap(bStack, sizeof(buffStack)) == -1)
    {
        fprintf(stderr, "%s could not unmap: %s", SHM_NAME_1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (close(shm) == -1)
    {
        fprintf(stderr, "%s could not be closed: %s", SHM_NAME_1, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void setUpSemaphores(void)
{

    if ((usedSem = sem_open(SEM1, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s could not be opened: %s", SEM1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((freeSem = sem_open(SEM2, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s could not be opened: %s", SEM2, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((mutexSem = sem_open(SEM3, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s could not be opened: %s", SEM3, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void tearDownSemaphores(void)
{
    if (sem_close(usedSem) == -1)
    {
        fprintf(stderr, "%s could not be closed: %s", SEM1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (sem_close(freeSem) == -1)
    {
        fprintf(stderr, "%s could not be closed: %s", SEM2, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (sem_close(mutexSem) == -1)
    {
        fprintf(stderr, "%s could not be closed: %s", SEM3, strerror(errno));
        exit(EXIT_FAILURE);
    }
}
