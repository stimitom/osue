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

static void writeToBuffer(solution val)
{
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

static void setSemaphores()
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
}

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

static void setRandomVertexColour(vertex *v)
{
    int random = (rand() % 3) + 1;
    v->colour = random;
}

static void assignGraphColouring(vertex *vertices, int numberOfVertices)
{
    for (int m = 0; m < numberOfVertices; m++)
    {
        setRandomVertexColour(&vertices[m]);
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

    if (close(shmfd) == -1)
    {
        fprintf(stderr, "%s: shared memory file descriptor could not be closed: %s\n", pgm_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    sem_close(free_sem);
    sem_close(used_sem);
    sem_close(mutex_sem); 
}

static void findEdgesToBeRemoved(vertex *vertices, int numberOfVertices, edge *edges, long numberOfEdges, int maxSolutionSize)
{
    printf("max sol size %d \n", maxSolutionSize);
    edge removeEdgesMax[sizeof(edges[0]) * 8];
    int numberOfEdgesToBeRemoved = maxSolutionSize + 1;
    while (numberOfEdgesToBeRemoved > maxSolutionSize)
    {
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
        printf("Edge: %ld-%ld needs to be removed\n", removeEdges[i].v1.id, removeEdges[i].v2.id);
    }

    solution s = {{removeEdges[0], removeEdges[1], removeEdges[2], removeEdges[3], removeEdges[4], removeEdges[5], removeEdges[6], removeEdges[7]}, numberOfEdgesToBeRemoved};
    writeToBuffer(s);
    if (numberOfEdgesToBeRemoved == 0)
    {
        printf("Found Graph Colouring.\n");
    }
}

int main(int argc, char *argv[])
{
    pgm_name = argv[0];
    if (argc > 1)
    {
        vertex *vertices;
        //Set vertices array to maximum possible size
        vertices = malloc(sizeof(vertex) * ((argc - 1) * 2));
        if (vertices == NULL)
        {
            fprintf(stderr, "malloc for vertices failed.\n");
            exit(EXIT_FAILURE);
        }
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

        setSharedMemory();
        setSemaphores();

        int maxSolutionSize = 8;
        while (maxSolutionSize >= 0)
        {
            findEdgesToBeRemoved(vertices, numberOfVertices, edges, (sizeof(edges) / sizeof(edges[0])), maxSolutionSize--);
        }

        //Cleanup
        cleanUp();
        free(vertices);
    }
    else
    {
        fprintf(stderr, "%s: needs at least one edge as input.\n", pgm_name);
        exit(EXIT_FAILURE);
    }
}