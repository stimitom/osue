/**
 * @file supervisor.c
 * @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief A programm printing solutions to the 3 colour problem that are calculated by generators.
 * 
 * @details This program sets up the shared-memory and semaphores needed for the generators. 
 * It surveils the shared memory and prints out the found solutions. 
 * It also makes it possible to safely terminate the supervisor process and all running generator processes with an interrupt.
 * 
 **/

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
#include <signal.h>

static char *pgm_name;

circBuff *cBuff;
sem_t *used_sem;
sem_t *free_sem;
sem_t *mutex_sem;
sem_t *generator_count_sem;

int shmfd;
volatile sig_atomic_t quit = 0;
int rd_pos = 0;
int currentBestSolutionSize = 9;

/**
 * readFromBuffer function
 * @brief This function reads solutions from the shared memory and prints them to stdout.
 * @details At first it checks if there are solutions left which it has not read yet and waits if there are none.
 * It checks if an Signal interrupted the waiting and exits if true.
 * It then reads a solution from the read position and increases the read position.
 * If the solution read is smaller than the best solution the solution is printed.
 * If the solution read has 0 removals the process terminates all generator by setting the termination flag.
 * @return the size of the found solution.
 */
static int readFromBuffer(void)
{

    if ((sem_wait(used_sem) == -1))
    {
        if (errno == EINTR)
        {
            if (quit)
            {
                exit(EXIT_SUCCESS);
            }
        }

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
        //Terminates all generators
        cBuff->terminate = 1;
        printf("Graph Colouring found.\n");
    }

    return val.solutionSize;
}

/**
 * initializeSemaphores function
 * @brief This function initializes the semaphores needed for the communication with the generator processes.
 * @details Opens the four needed semaphores: 
 * used_sem inidcates the number of used spaces in the buffer.
 * free_sem indicates the number of free spaces in the buffer.
 * mutex_sem indicates if another process is currently writing to the buffer.
 * generator_count_sem counts the number of generator processes.
 * @return void
 */
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

    if ((generator_count_sem = sem_open(SEM_4, O_CREAT | O_EXCL, 0600, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s: generator_count_sem semaphore could not be initialized: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 *  setUpSharedMemory function
 * @brief This function sets up the shared memory needed for the communication with the generator processes.
 * @details Opens the shared memory truncates its size and maps it to cBuff.
 * @return void
 */
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

/**
 *  cleanUp function
 * @brief A cleanup function.
 * @details This function unmaps, unlinks and closes the shared memory and closes and unlinks all semaphores.
 * @return void
 */
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
    sem_close(generator_count_sem);
    sem_unlink(SEM_1);
    sem_unlink(SEM_2);
    sem_unlink(SEM_3);
    sem_unlink(SEM_4);
}

/**
 *  terminate function
 * @brief The function always called upon terminating the process.
 * @details This function terminates all generators by getting the number of generators and increasing the needed semaphores
 * by it. That way no generator can remain stuck waiting. 
 * The termination flag is set and the cleanup is called.
 * @return void
 */
static void terminate(void)
{
    int numberOfGenerators;
    if (sem_getvalue(generator_count_sem, &numberOfGenerators) == -1)
    {
        fprintf(stderr, "%s: could not read generator_count_sem semaphore value: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < numberOfGenerators; i++)
    {
        sem_post(mutex_sem);
        sem_post(free_sem);
    }
    cBuff->terminate = 1;
    cleanUp();
}

/**
 *  handle_signal function
 * @brief A signal handler
 * @details Sets the global volatile flag quit to 1.
 * @param signal 
 * @return void
 */
static void handle_signal(int signal)
{
    quit = 1;
}

/**
 *  setSignalHandling function
 * @brief Sets up the signal handlers.
 * @details Sets the signal handler for the SIGINT and SIGTERM signals.
 * @return void
 */
static void setSignalHandling(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/**
 * main function - programm entry point
 * @brief Sets up signalHandling, sharedMemory and semaphores and starts the surveillance of the buffer.
 * @details Sets up signalHandling, sharedMemory and semaphores and 
 * then continues to read from the buffer until a solution of size 0 is found.
 * Terminates safely if the global volatile quit is set.
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char *argv[])
{
    pgm_name = argv[0];

    if (atexit(terminate) < 0)
    {
        fprintf(stderr, "%s: Could not set exit function.", pgm_name);
        exit(EXIT_FAILURE);
    }
    setSignalHandling();

    while (!quit)
    {
        setUpSharedMemory();

        initializeSemaphores();

        int lookingForSolution = 1;
        while (lookingForSolution)
        {
            lookingForSolution = readFromBuffer();
        }
    }

    return EXIT_SUCCESS;
}
