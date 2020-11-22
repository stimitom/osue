/**
 * @file generator.c
 * @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief A generator calculating solutions to the 3 coloring problem.
 * 
 * @details This program takes a number of edges as input and writes a solution, that is a set of edges that needs to be removed
 * to solve the 3 coloring problem of the given graph, to a circular buffer.
 * 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include "generator.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

static char *pgm_name;
int shmfd;
circBuff *cBuff;
sem_t *used_sem;
sem_t *free_sem;
sem_t *mutex_sem;
sem_t *generator_count_sem;
int countedThisGenerator = 0;
vertex *vertices;

/**
 *  setSemaphores function
 * @brief This function sets up the semaphores needed for the communication with the supervisor process.
 * @details Opens the four needed semaphores: 
 * used_sem inidcates the number of used spaces in the buffer.
 * free_sem indicates the number of free spaces in the buffer.
 * mutex_sem indicates if another process is currently writing to the buffer.
 * generator_count_sem counts the number of generator processes.
 * @return void
 */
static void setSemaphores(void)
{
    if ((used_sem = sem_open(SEM_1, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s: used_sem semaphore could not be initialized: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((free_sem = sem_open(SEM_2, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s: free_sem semaphore could not be initialized: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((mutex_sem = sem_open(SEM_3, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s: mutex_sem semaphore could not be initialized: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((generator_count_sem = sem_open(SEM_4, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s: generator_count_sem semaphore could not be initialized: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 *  setSharedMemory function
 * @brief This function sets up the shared memory needed for the communication with the supervisor process.
 * @details Opens the shared memory and maps it to cBuff.
 * @return void
 */
static void setSharedMemory(void)
{
    if ((shmfd = shm_open(SHM_NAME, O_RDWR, 0600)) == -1)
    {
        fprintf(stderr, "%s: shared memory could not be opened: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((cBuff = mmap(NULL, sizeof(cBuff), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "%s: shared memory could not be mapped: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 *  setRandomVertexColour function
 * @brief Generates a random colouring for a vertex.
 * @details This function takes a vertex as input and sets a random colouring (out of 3 possible colours) to it.
 * @param v vertex to be coloured.
 * @return void
 */
static void setRandomVertexColour(vertex *v)
{
    int random = (rand() % 3) + 1;
    v->colour = random;
}

/**
 *  assignGraphColouring function
 * @brief Generates a random colouring for a graph.
 * @details This function takes an array of vertices and calls setRandomVertexColour on each vertex.
 * @param vertices vertex array to be coloured.
 * @param numberOfVertices vertex array size.
 * @return void
 */
static void assignGraphColouring(vertex *vertices, int numberOfVertices)
{
    for (int m = 0; m < numberOfVertices; m++)
    {
        setRandomVertexColour(&vertices[m]);
    }
}

/**
 *  cleanUp function
 * @brief A cleanup function called upon terminating.
 * @details This function unmaps and closes the shared memory, closes all semaphores and frees the allocated spaces for the vertices array.
 * @return void
 */
static void cleanUp(void)
{
    printf("Exit mehthod called.\n");
    if (munmap(cBuff, sizeof(cBuff)) == -1)
    {
        fprintf(stderr, "%s: shared memory file descriptor could not be unmapped: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (close(shmfd) == -1)
    {
        fprintf(stderr, "%s: shared memory file descriptor could not be closed: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    sem_close(free_sem);
    sem_close(used_sem);
    sem_close(mutex_sem);
    sem_close(generator_count_sem);

    free(vertices);
}

/**
 *  writeToBuffer function
 * @brief This function writes a found solution to the shared memory.
 * @details First the function checks if the termination flag is set and terminates if true.
 * If the generator aims to write for the first time , the counter semaphore is increased.
 * It then checks if another process is currently writing and waits if true.
 * It then checks if there is still space left in the buffer and waits if there is none.
 * It then writes the solution to the buffer, increases the writeposition and finishes by setting the 
 * mutex semaphore free again.
 * @param val solution to be written to the buffer.
 * @return void
 */
static void writeToBuffer(solution val)
{
    if (cBuff->terminate)
    {
        printf("exited\n");
        exit(EXIT_SUCCESS);
    }
    if (!countedThisGenerator)
    {
        if ((sem_post(generator_count_sem)) == -1)
        {
            fprintf(stderr, "%s: generator_count_sem semaphore could not be incremented: %s\n", pgm_name, strerror(errno));
            exit(EXIT_FAILURE);
        }
        countedThisGenerator++;
    }

    if (sem_wait(mutex_sem) == -1)
    {
        fprintf(stderr, "%s: mutex_sem semaphore could not be decremented: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((sem_wait(free_sem) == -1))
    {
        fprintf(stderr, "%s: free_sem semaphore could not be decremented: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    cBuff->array[cBuff->writePosition] = val;
    if ((sem_post(used_sem)) == -1)
    {
        fprintf(stderr, "%s: used_sem semaphore could not be incremented: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    cBuff->writePosition += 1;
    cBuff->writePosition %= MAX_BUFF_SIZE;

    if (sem_post(mutex_sem) == -1)
    {
        fprintf(stderr, "%s: mutex_sem semaphore could not be incremented: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 *  findEdgesToBeRemoved function
 * @brief This function calculates solutions of the 3 colouring problem of a given graph.
 * @details The function creates an array (removeEdegesMax) with the size of the maximum number of edges that can be removed for a solution: 8.
 * At first it is always checked if the supervisor wants to terminate the process.
 * A random graph colouring is assigned and if two vertices that build an edge have the same colour, 
 * the edge is added to removeEdgesMax and the counter for the numberOfEdges to be removed is increased.
 * If a solution with less edges than the given maxSolutionSize was found a solution struct is created and 
 * written to the buffer.
 * @param vertices array of all vertices in the graph.
 * @param numberOfVertices size of param:vertices.
 * @param edges array of all edges in the graph.
 * @param numberOfEdged size of param:edges.
 * @param maxSolutionSize maximum solution size to be searched for.
 * @return void
 */
static void findEdgesToBeRemoved(vertex *vertices, int numberOfVertices, edge *edges, long numberOfEdges, int maxSolutionSize)
{
    edge removeEdgesMax[sizeof(edges[0]) * 8];
    int numberOfEdgesToBeRemoved = maxSolutionSize + 1;
    while (numberOfEdgesToBeRemoved > maxSolutionSize)
    {
        if (cBuff->terminate)
        {
            printf("exited\n");
            exit(EXIT_SUCCESS);
        }
        assignGraphColouring(vertices, numberOfVertices);
        numberOfEdgesToBeRemoved = 0;

        for (long i = 0; i < numberOfEdges; i++)
        {
            int v1Colour = 0;
            int v2Colour = 0;
            for (int j = 0; j < numberOfVertices; j++)
            {
                if (edges[i].v1.id == vertices[j].id)
                {
                    v1Colour = vertices[j].colour;
                }
                if (edges[i].v2.id == vertices[j].id)
                {
                    v2Colour = vertices[j].colour;
                }
                if (v1Colour && v2Colour)
                {
                    break;
                }
            }
            if (v1Colour && (v1Colour == v2Colour))
            {
                removeEdgesMax[numberOfEdgesToBeRemoved++] = edges[i];
            }
        }
    }
    edge removeEdges[8];
    for (int i = 0; i < numberOfEdgesToBeRemoved; i++)
    {
        removeEdges[i] = removeEdgesMax[i];
    }

    solution s = {{removeEdges[0], removeEdges[1], removeEdges[2], removeEdges[3], removeEdges[4], removeEdges[5], removeEdges[6], removeEdges[7]}, numberOfEdgesToBeRemoved};
    writeToBuffer(s);
}

/**
 *  main function - program entry function
 * @brief This function parses the input arguments, calls the setUp functions for the inter-process communication and starts the search for solutions.
 * @details Sets an exit function at the beginning. 
 * Then allocates the maximum needed space for the vertices. 
 * Parses the arguments (edges) by using strtol function. 
 * Checks if the part of an edge before and after the "-"-sign are well formed exits if not.
 * Adds each vertex to the vertices array if its not already in it. After all vertices are assigned 
 * to the array it is truncated to the actual size.
 * Adds each parsed edge to an edge array.
 * It then calls the setup Methods for the shared memory and semaphores and calls the solution calculation with ever decreasing maximum sizes
 * until a graph colouring without removals is found.
 * @param argc argument counter
 * @param argv argument vector
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char *argv[])
{
    pgm_name = argv[0];

    if (atexit(cleanUp) < 0)
    {
        fprintf(stderr, "%s: Could not set exit function.\n", pgm_name);
        exit(EXIT_FAILURE);
    }

    if (argc <= 1)
    {
        fprintf(stderr, "%s: needs at least one edge as input.\n", pgm_name);
        exit(EXIT_FAILURE);
    }

    //Set vertices space to maximum possible size
    vertices = malloc(sizeof(vertex) * ((argc - 1) * 2));
    if (vertices == NULL)
    {
        fprintf(stderr, "malloc for vertices failed.\n");
        exit(EXIT_FAILURE);
    }

    // Argument parsing

    int numberOfVertices = 0;
    edge edges[argc - 1];

    for (int i = 1; i < argc; i++)
    {
        long int v1;
        char *nptr = argv[i];
        char *endptr;
        v1 = strtol(nptr, &endptr, 10);

        if (v1 == LONG_MIN || v1 == LONG_MAX)
        {
            fprintf(stderr, "%s: input could not be parsed.The identifier before the '-'sign of at least one edge is too big to be computed. %s\n", pgm_name, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (endptr != NULL)
        {
            if (endptr == nptr || endptr[0] != '-')
            {
                fprintf(stderr, "%s: input could not be parsed. At least one edge is malformed before the '-' sign.\n", pgm_name);
                exit(EXIT_FAILURE);
            }
        }

        long v2;
        char *endptr2;
        v2 = strtol(endptr + 1, &endptr2, 10);

        if (v2 == LONG_MIN || v2 == LONG_MAX)
        {
            fprintf(stderr, "%s: input could not be parsed. The identifier after the '-' sign of at least one edge is too big to be computed. %s\n", pgm_name, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (endptr2 != NULL)
        {
            if (endptr2 == (endptr + 1) || endptr2[0] != '\0')
            {
                fprintf(stderr, "%s: input could not be parsed. At least one edge is malformed after the '-' sign.\n", pgm_name);
                exit(EXIT_FAILURE);
            }
        }

        vertex vert1 = {v1, EMTPY};
        vertex vert2 = {v2, EMTPY};

        //Set first vertex.
        if (numberOfVertices == 0)
        {
            vertices[0] = vert1;
            numberOfVertices++;
        }

        //Check for each vertex if it already exists in the array. Add if it does not.
        int vert1InArray = 0;
        int vert2InArray = 0;
        for (int l = 0; l < numberOfVertices; l++)
        {
            if (vertices[l].id == vert1.id)
            {
                vert1InArray = 1;
            }

            if (vertices[l].id == vert2.id)
            {
                vert2InArray = 1;
            }
        }

        if (vert1InArray == 0)
        {
            vertices[numberOfVertices] = vert1;
            numberOfVertices++;
        }
        if (vert2InArray == 0)
        {
            vertices[numberOfVertices] = vert2;
            numberOfVertices++;
        }

        //Add to edges
        edge e = {vert1, vert2};
        edges[i - 1] = e;
    }

    //Set vertices to exact size
    if (realloc(vertices, sizeof(vertex) * numberOfVertices) == NULL)
    {
        fprintf(stderr, "realloc for vertices failed.\n");
        exit(EXIT_FAILURE);
    }

    //ENDOF ARGUMENT PARSING

    setSharedMemory();
    setSemaphores();

    // Calculates solutions until a graph colouring is found.
    int maxSolutionSize = 8;
    while (maxSolutionSize >= 0)
    {
        findEdgesToBeRemoved(vertices, numberOfVertices, edges, (sizeof(edges) / sizeof(edges[0])), maxSolutionSize--);
    }

    exit(EXIT_SUCCESS);
}