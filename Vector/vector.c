#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "vector.h"

struct Vector
{
    void **m_items;
    size_t m_originalSize; // the vector should not be smaller then this size
    size_t m_size;         // the current size (capacity) of the array
    size_t m_nItems;       // number of elemets
    size_t m_blockSize;    // the vector will grow or shrink on demand by this size
};




Vector *VectorCreate(size_t _initialCapacity, size_t _blockSize)
{

    if (!_initialCapacity && !_blockSize)
        return NULL;

    /* Guard against wrapped negatives: size_t is unsigned, so passing a
       negative int (e.g. -1) silently becomes SIZE_MAX.  If blockSize is
       that large, the realloc in VectorAppend would compute
       sizeof(void*) * (m_size + blockSize), which overflows back to a tiny
       number and causes a heap buffer overflow with no visible error. */
    if (_blockSize > SIZE_MAX / sizeof(void *))
        return NULL;

    Vector *vector = (Vector *)malloc(sizeof(Vector));
    if (!vector)
        return NULL;

    vector->m_items = (void **)malloc(sizeof(void *) * _initialCapacity);

    if (!vector->m_items)
    {
        free(vector); // clean up first malloc before leaving
        return NULL;
    }

    vector->m_blockSize = _blockSize;
    vector->m_originalSize = _initialCapacity;
    vector->m_nItems = 0;
    vector->m_size = _initialCapacity;

    return vector;
}

void VectorDestroy(Vector **_vector, void (*_elementDestroy)(void *_item)) // Vector **_vector beacuse we want to modify the pointer itself
{
    if (!_vector || !*_vector)
        return;

    if (_elementDestroy)
    {
        for (size_t i = 0; i < (*_vector)->m_nItems; i++)
        {
            _elementDestroy((*_vector)->m_items[i]);
        }
    }

    free((*_vector)->m_items);
    free(*_vector);
    *_vector = NULL;
}

VectorResult VectorAppend(Vector *_vector, void *_item)
{
    if (!_vector)
    {
        return VECTOR_UNITIALIZED_ERROR;
    }

    if (!_item)
    {
        return ITEM_UNITIALIZED_ERROR;
    }

    if (_vector->m_size == _vector->m_nItems)
    {
        if (_vector->m_blockSize == 0)
            return VECTOR_ALLOCATION_ERROR;

        void **realloced_vector = realloc(_vector->m_items, sizeof(void *) *
                                                                (_vector->m_size + _vector->m_blockSize));
        if (!realloced_vector)
        {
            return VECTOR_ALLOCATION_ERROR;
        }
        _vector->m_items = realloced_vector;
        _vector->m_size += _vector->m_blockSize;
    }

    _vector->m_items[_vector->m_nItems++] = _item;

    return VECTOR_SUCCESS;
}

VectorResult VectorRemove(Vector *_vector, void **_pValue)
{
    if (!_vector)
        return VECTOR_UNITIALIZED_ERROR;

    if (_vector->m_nItems == 0)
        return VECTOR_IS_EMPTY;

    if (!_pValue)
        return VALUE_IS_NULL;

    *_pValue = _vector->m_items[_vector->m_nItems - 1];
    _vector->m_nItems--;

    size_t new_size = _vector->m_size - _vector->m_blockSize;

    if (_vector->m_nItems < new_size && _vector->m_size > _vector->m_originalSize)
    {
        void **realloced = realloc(_vector->m_items, sizeof(void *) * new_size);
        if (!realloced)
            return VECTOR_ALLOCATION_ERROR;

        _vector->m_items = realloced;
        _vector->m_size = new_size;
    }

    return VECTOR_SUCCESS;
}

VectorResult VectorGet(const Vector *_vector, size_t _index, void **_pValue)
{
    if (!_vector)
    {
        return VECTOR_UNITIALIZED_ERROR;
    }

    if (_index >= _vector->m_nItems)
    {
        return VECTOR_INDEX_OUT_OF_BOUNDS_ERROR;
    }

    if (!_pValue)
        return VALUE_IS_NULL;

    *_pValue = _vector->m_items[_index];

    return VECTOR_SUCCESS;
}

VectorResult VectorSet(Vector *_vector, size_t _index, void *_value)
{
    if (!_vector)
        return VECTOR_UNITIALIZED_ERROR;

    if (_index >= _vector->m_nItems)
        return VECTOR_INDEX_OUT_OF_BOUNDS_ERROR;

    _vector->m_items[_index] = _value;

    return VECTOR_SUCCESS;
}

size_t VectorSize(const Vector *_vector)
{
    if (!_vector)
    {
        return VECTOR_UNITIALIZED_ERROR;
    }

    return _vector->m_nItems;
}

size_t VectorCapacity(const Vector *_vector)
{

    if (!_vector)
    {
        return VECTOR_UNITIALIZED_ERROR;
    }

    return _vector->m_size;
}

size_t VectorForEach(const Vector *_vector, VectorElementAction _action, void *_context)
{
    size_t i;

    if (!_vector)
    {
        return 0;
    }

    if (!_action)
    {
        return 0;
    }

    for (i = 0; i < _vector->m_nItems; i++)
    {
        if (_action(_vector->m_items[i], i, _context) == 0)
        {
            break;
        }
    }
    return i;
}
// number of times the user functions was invoked
//  * equevallent to:
//  *      for(i = 0; i < VectorSize(v); ++i){
//  *             VectorGet(v, i, &elem);
//  *             if(_action(elem, i, _context) == 0)
//  *					break;
//  *      }
//  *		return i;
//  */