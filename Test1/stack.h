typedef struct {
    char *item;
} STACKITEM;

typedef struct {
    int capacity, top;
    STACKITEM *items;
} STACK;

STACK* createStack(void);
STACKITEM* createStackItem(char *item);
void push(STACKITEM *item, STACK *stack);
void pop(STACK *stack);
void display(STACK *stack);
