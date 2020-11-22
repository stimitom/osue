#define SHM_NAME "/genshm"
#define MAX_BUFF_SIZE 10
#define SEM_1 "/sem_1"
#define SEM_2 "/sem_2"
#define SEM_3 "/sem_3"
#define SEM_4 "/sem_4"

enum colour
{
    EMTPY,
    RED,
    GREEN,
    BLUE
};

typedef struct vert
{
    long int id;
    enum colour colour;
} vertex;

typedef struct edg
{
    vertex v1;
    vertex v2;
} edge;

typedef struct sol {
    edge edges[8];
    int solutionSize;
} solution;

typedef struct buff {
    solution array[MAX_BUFF_SIZE]; 
    int writePosition;
    int numberOfGenerators; 
    int terminate;
} circBuff;



