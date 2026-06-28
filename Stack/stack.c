#include <stdio.h>
#include <stdlib.h>
#include "../Vector/vector.h"
#include "stack.h"

struct Stack
{
    Vector *vector;
};

static StackResult vectorToStackResult(VectorResult res)
{
    switch (res)
    {
    case VECTOR_SUCCESS:
        return STACK_SUCCESS;
    case VECTOR_UNITIALIZED_ERROR:
        return STACK_UNITIALIZED_ERROR;
    case VECTOR_ALLOCATION_ERROR:
        return STACK_ALLOCATION_ERROR;
    case VECTOR_INDEX_OUT_OF_BOUNDS_ERROR:
        return STACK_IS_EMPTY;
    case VECTOR_IS_EMPTY:
        return STACK_IS_EMPTY;
    case ITEM_UNITIALIZED_ERROR:
        return STACK_ITEM_UNITIALIZED_ERROR;
    case VALUE_IS_NULL:
        return STACK_VALUE_IS_NULL;
    }
    return STACK_SUCCESS;
}

Stack *StackCreate(size_t _initialCapacity, size_t _blockSize)
{
    if (!_initialCapacity && !_blockSize)
    {
        return NULL;
    }

    Stack *stack = (Stack *)malloc(sizeof(Stack));
    if (!stack)
    {
        return NULL;
    }

    stack->vector = VectorCreate(_initialCapacity, _blockSize);
    if (!stack->vector)
    {
        free(stack);
        return NULL;
    }

    return stack;
}

void StackDestroy(Stack **_stack, void (*_elementDestroy)(void *_item))
{

    if (!_stack || !*_stack)
    {
        return;
    }

    // no need to check if the function pointer is null
    //  because the check will be anyway inside vector destroyed
    VectorDestroy(&((*_stack)->vector), _elementDestroy);
    free(*_stack);
    *_stack = NULL;
}

StackResult StackPush(Stack *_stack, void *_item)
{
    if (!_stack)
        return STACK_UNITIALIZED_ERROR;
    if (!_item)
    {
        return STACK_ITEM_UNITIALIZED_ERROR;
    }

    return vectorToStackResult(VectorAppend(_stack->vector, _item));
}

StackResult StackPop(Stack *_stack, void **_pValue)
{
    if (!_stack)
        return STACK_UNITIALIZED_ERROR;

    if (!_pValue)
        return STACK_VALUE_IS_NULL;

    return vectorToStackResult(VectorRemove(_stack->vector, _pValue));
}

StackResult StackTop(Stack *_stack, void **_pValue)
{
    if (!_stack)
        return STACK_UNITIALIZED_ERROR;

    if (!_pValue)
        return STACK_VALUE_IS_NULL;

    if (VectorSize(_stack->vector) == 0)
        return STACK_IS_EMPTY;

    return vectorToStackResult(VectorGet(_stack->vector, VectorSize(_stack->vector) - 1, _pValue));
}

size_t StackSize(const Stack *_stack)
{
    if (!_stack)
        return 0;

    return VectorSize(_stack->vector);
}

size_t StackCapacity(const Stack *_stack)
{
    if (!_stack || !_stack->vector)
    {
        return 0;
    }

    return VectorCapacity(_stack->vector);
}

int StackIsEmpty(Stack *_stack)
{
    if (!_stack || !_stack->vector)
    {
        return 1;
    }

    if (VectorSize(_stack->vector) == 0)
    {
        return 1;
    }

    return 0;
}

size_t StackPrint(const Stack *_stack, StackElementAction _action, void *_context)
{
    if (!_stack || !_action)
        return 0;

    return VectorForEach(_stack->vector, (VectorElementAction)_action, _context);
}
