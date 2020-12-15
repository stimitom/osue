#define SHM_NAME_1 "/myshm"
#define SEM1 "/sem1"
#define SEM2 "/sem2"
#define SEM3 "/sem3"
#define STACKSIZE 10


typedef struct 
{
    int a;
    int b; 
} stackElem;


typedef struct
{
 int top; 
 stackElem items[STACKSIZE];
}buffStack;

