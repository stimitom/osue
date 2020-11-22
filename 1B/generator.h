/**
 * @file generator.h
 * @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
 * @date 22.11.2020
 *
 * @brief The header file for generator.c and supervisor.c
 * 
 * @details Contains the definitions of semaphore names, buffer names and all needed structs.
 * 
 **/

#define SHM_NAME "/genshm"
#define MAX_BUFF_SIZE 10
#define SEM_1 "/01551905_sem_1"
#define SEM_2 "/01551905_sem_2"
#define SEM_3 "/01551905_sem_3"
#define SEM_4 "/01551905_sem_4"

// colour (represented as ints) for a vertex
enum colour
{
    EMTPY,
    RED,
    GREEN,
    BLUE
};

// vertex struct containing its id and its colour
typedef struct vert
{
    long int id;
    enum colour colour;
} vertex;

// edge struct containing two vertices
typedef struct edg
{
    vertex v1;
    vertex v2;
} edge;

// solution struct containing an array of size 8 of edges and the actual size of the solution
typedef struct sol
{
    edge edges[8];
    int solutionSize;
} solution;

// circBuff struct containing an array of solutions, the write position, and a termination flag
typedef struct buff
{
    solution array[MAX_BUFF_SIZE];
    int writePosition;
    int terminate;
} circBuff;
