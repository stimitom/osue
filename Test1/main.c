
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "stack.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shared.h"
#include <semaphore.h>

#define RANDOMNUMBER 34

typedef struct nd
{
    char data[10];
    struct nd *next;
} node;

char *pgm_name;
void append(void);
void printList(node *head);
void cleanUp(void);
void linkedList(void);
void setExit(void);
void usage(void);
void handleSignal(int signal);
void stack(void);
void writeToFileAndReverse(FILE *file, char *line);
void readAndWriteReverseToFile(FILE *file);

void setUpSharedMemory(void);
void tearDownSharedMemory(void);
void setUpSemaphores(void);
void tearDownSemaphores(void);
int readFromBuffStack(void);
void tearDown(void);
static void setSignalHandling(void);
static void handle_signal(int signal);

int shm;
buffStack *bStack;
sem_t *usedSem;
sem_t *freeSem;
sem_t *mutexSem;
volatile sig_atomic_t quit = 0;

int main(int argc, char *argv[])
{
    pgm_name = argv[0];
    setSignalHandling();

    if (atexit(tearDown) != 0)
    {
        fprintf(stderr, "atexit failed\n");
    }

    setUpSharedMemory();
    setUpSemaphores();

    int result = 0;
    while(!quit && result != 7){
        result = readFromBuffStack();
    }
    if (result == 7)
    {
        printf("Found the 7!\n");
    }
    
   
}

void tearDown(void)
{
    tearDownSharedMemory();
    tearDownSemaphores();
}

int readFromBuffStack(void)
{
    if (sem_wait(usedSem) == -1)
    {
        if (errno == EINTR)
        {
            if (quit)
            {
                exit(EXIT_SUCCESS);
            }
        }
        fprintf(stderr, "%s something went wrong with sem wait of %s: %s", pgm_name, SEM1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    stackElem e = bStack->items[bStack->top--];

    if (sem_post(freeSem) == -1)
    {
        fprintf(stderr, "%s something went wrong with sem post of %s: %s", pgm_name, SEM2, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return e.a + e.b;
}

void setUpSharedMemory(void)
{

    if ((shm = shm_open(SHM_NAME_1, O_CREAT | O_RDWR, 0600)) == -1)
    {
        fprintf(stderr, "%s could not be opened: %s", SHM_NAME_1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((ftruncate(shm, sizeof(bStack)) == -1))
    {
        fprintf(stderr, "%s could not be truncated: %s", SHM_NAME_1, strerror(errno));
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

    if (shm_unlink(SHM_NAME_1) == -1)
    {
        fprintf(stderr, "%s could not be unlinked: %s", SHM_NAME_1, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void setUpSemaphores(void)
{

    if ((usedSem = sem_open(SEM1, O_CREAT | O_EXCL, 0600, 0)) == SEM_FAILED)
    {
        fprintf(stderr, "%s could not be opened: %s", SEM1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((freeSem = sem_open(SEM2, O_CREAT | O_EXCL, 0600, STACKSIZE)) == SEM_FAILED)
    {
        fprintf(stderr, "%s could not be opened: %s", SEM2, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((mutexSem = sem_open(SEM3, O_CREAT | O_EXCL, 0600, 1)) == SEM_FAILED)
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

    if (sem_unlink(SEM1) == -1)
    {
        fprintf(stderr, "%s could not be closed: %s", SEM1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(SEM2) == -1)
    {
        fprintf(stderr, "%s could not be closed: %s", SEM1, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(SEM3) == -1)
    {
        fprintf(stderr, "%s could not be closed: %s", SEM1, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void readAndWriteReverseToFile(FILE *file)
{
    char *line = NULL;
    size_t n = 0;
    ssize_t linesize;

    while ((linesize = getline(&line, &n, stdin)) != -1)
    {
        if (line[linesize - 1] == '\n')
        {
            line[linesize - 1] = '\0';
            --linesize;
        }
        writeToFileAndReverse(file, line);
    }
    free(line);
}

void writeToFileAndReverse(FILE *file, char *line)
{
    if (fputs(line, file) == EOF)
    {
        fprintf(stderr, "Writing to the file went wrong: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return;
}

void stack(void)
{
    STACK *stack = createStack();

    STACKITEM *item1 = createStackItem("sdfasd");
    STACKITEM *item2 = createStackItem("s ");
    STACKITEM *item3 = createStackItem("geht es");
    STACKITEM *item4 = createStackItem("dir ?");

    push(item1, stack);
    push(item2, stack);
    push(item3, stack);

    display(stack);

    pop(stack);
    push(item4, stack);
    display(stack);
    pop(stack);
    pop(stack);
    pop(stack);
    display(stack);
}

void stuff(int argc, char *argv[])
{
    setExit();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handleSignal;

    sigaction(SIGINT, &sa, NULL);

    int c;
    int o_count;
    char *o_option;
    if (argc == 1)
    {
        printf("No options provided.\n");
    }
    else
    {
        while ((c = getopt(argc, argv, "ao:")) != -1)
        {
            switch (c)
            {
            case 'a':
                printf("Option a provided.\n");
                break;
            case 'o':
                o_count++;
                o_option = optarg;
                printf("Option o with parameter %s provided.\n", optarg);
                break;
            case '?':
                usage();
                break;
            default:
                break;
            }
        }
    }

    FILE *file;
    if (o_count > 0)
    {
        if (o_count == 1)
        {
            if ((file = fopen(o_option, "a+")) == NULL)
            {
                fprintf(stderr, "Opening file failed: %s ", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fprintf(stderr, "Option o was provided too often.");
            usage();
        }
    }
}

void handleSignal(int signal)
{
    printf("SIgnal\n");
}

void usage(void)
{
    fprintf(stderr, "Usage: main [-o] filename [-a]\n");
    exit(EXIT_FAILURE);
}

void setExit(void)
{
    if (atexit(cleanUp) != 0)
    {
        fprintf(stderr, "%s somthing went wrong with setting the exit function.", pgm_name);
        exit(EXIT_FAILURE);
    }
}

void linkedList(void)
{

    node *head = NULL;
    node *node1 = NULL;
    node *node2 = NULL;

    head = malloc(sizeof(node));
    if (head == NULL)
    {
        fprintf(stderr, "%s somthing went wrong with malloc.", pgm_name);
        exit(EXIT_FAILURE);
    }

    if ((node1 = malloc(sizeof(node))) == NULL)
    {
        fprintf(stderr, "%s somthing went wrong with malloc.", pgm_name);
        exit(EXIT_FAILURE);
    }

    if ((node2 = malloc(sizeof(node))) == NULL)
    {
        fprintf(stderr, "%s somthing went wrong with malloc.", pgm_name);
        exit(EXIT_FAILURE);
    }

    strcpy(head->data, "hello");
    head->next = node1;

    strcpy(node1->data, "you");
    node1->next = node2;

    strcpy(node2->data, "suck");
    node2->next = NULL;

    printList(head);

    free(head);
    free(node1);
    free(node2);
}

void printList(node *head)
{
    node *p = head;
    while (p != NULL)
    {
        printf("%s ", p->data);
        p = p->next;
    }
    printf("\n");
}

void cleanUp(void)
{
    printf("Goodbye\n");
}


static void handle_signal(int signal)
{
    quit = 1;
}

static void setSignalHandling(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}