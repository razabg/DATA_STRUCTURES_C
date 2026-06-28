#include "queue.h"

struct Queue
{
    void **m_queue;
    size_t m_size;
    size_t m_head; /* Index of head in m_que. */
    size_t m_tail; /* Index of tail in m_que. */
    size_t m_nItems;
};

Queue *QueueCreate(size_t _size)
{
    if (_size == 0)
    {
        return NULL;
    }

    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (!queue)
    {
        return NULL;
    }

    queue->m_queue = (void **)malloc(sizeof(void *) * _size);
    if (!queue->m_queue)
    {
        free(queue);
        return NULL;
    }

    queue->m_nItems = 0;
    queue->m_size = _size;
    queue->m_head = 0;
    queue->m_tail = 0;

    return queue;
}

void QueueDestroy(Queue **_queue, DestroyItem _itemDestroy)
{
    if (!_queue || !*_queue)
        return;

    if (_itemDestroy)
    {
        for (size_t i = 0; i < (*_queue)->m_nItems; i++)
        {
            _itemDestroy((*_queue)->m_queue[((*_queue)->m_head + i) % (*_queue)->m_size]);
        }
    }

    free((*_queue)->m_queue);
    free(*_queue);
    *_queue = NULL;
}

QueueResult QueueInsert(Queue *_queue, void *_item)
{
    if (!_queue)
    {
        return QUEUE_UNINITIALIZED_ERROR;
    }

    if (!_item)
    {
        return ITEM_IS_NULL;
    }

    if (_queue->m_size == _queue->m_nItems)
    {
        return QUEUE_OVERFLOW_ERROR;
    }

    _queue->m_queue[_queue->m_tail] = _item;
    _queue->m_tail = (_queue->m_tail + 1) % _queue->m_size;
    _queue->m_nItems++;

    return QUEUE_SUCCESS;
}

QueueResult QueueRemove(Queue *_queue, void **_item)
{
    if (!_queue)
        return QUEUE_UNINITIALIZED_ERROR;

    if (_queue->m_nItems == 0)
        return QUEUE_DATA_NOT_FOUND_ERROR;

    if (!_item)
        return QUEUE_DATA_UNINITIALIZED_ERROR;

    *_item = _queue->m_queue[_queue->m_head];
    _queue->m_head = (_queue->m_head + 1) % _queue->m_size;
    _queue->m_nItems--;

    return QUEUE_SUCCESS;
}

size_t QueueIsEmpty(Queue *_queue)
{
    if (!_queue)
    {
        return 1;
    }

    return _queue->m_nItems == 0;
}

size_t QueueForEach(Queue *_queue, ActionFunction _action, void *_context)
{
    size_t i;

    if (!_queue)
    {
        return 0;
    }

    if (!_action)
    {
        return 0;
    }

    for (i = 0; i < _queue->m_nItems; i++)
    {
        if (_action((_queue)->m_queue[((_queue)->m_head + i) % (_queue)->m_size], _context) == 0)
        {
            break;
        }
    }
    return i;
}
