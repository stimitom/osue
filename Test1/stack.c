
#include "stack.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

STACK *createStack(void)
{
    STACK *stack = (STACK *)malloc(sizeof(STACK));
    stack->capacity = -1;
    stack->top = -1;
    stack -> items = 0;
    return stack;
}

void push(STACKITEM *item, STACK *stack)
{
    if (stack->top == stack->capacity)
    {
        int newCap = stack->capacity + 10;
        STACKITEM *newPtr;
        if ((newPtr = (STACKITEM *) realloc(stack->items, newCap * sizeof(STACKITEM))) == NULL)
        {
            fprintf(stderr, "Realloc for stackitems failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        stack->items = newPtr;
        stack->capacity = newCap;
    }
    stack->top++;
    stack->items[stack->top] = *item;
}

void pop(STACK *stack)
{
    if (stack->top >= 0)
    {
        STACKITEM popped = stack->items[stack->top--];
        printf("Popped %s\n", popped.item);
    }else{
        printf("Stack is already empty.\n");
    }
}

void display(STACK *stack)
{
    if ((stack->top) == -1)
    {
        printf("Stack is empty.\n");
        return;
    }

    printf("STACK: \n");
    for (int i = 0; i <= stack->top; i++){
        printf("%d -- %s\n",i,stack->items[i].item);
    }
}

STACKITEM *createStackItem(char *content){
    STACKITEM *stackItem = (STACKITEM *) malloc (sizeof(STACKITEM));
    stackItem->item = content;
    stackItem-> item = "hallo";
    return stackItem; 
}