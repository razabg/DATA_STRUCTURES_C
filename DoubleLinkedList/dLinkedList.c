#include "dLinkedList.h"
#include <stdlib.h>

struct Node
{
    void *m_data;
    Node *m_next;
    Node *m_prev;
};

struct List
{
    Node m_head;
    Node m_tail;
};

List *ListCreate(void)
{

    List *list = (List *)malloc(sizeof(List));
    if (!list)
    {
        return NULL;
    }

    list->m_head.m_next = &list->m_tail;
    list->m_tail.m_prev = &list->m_head;

    list->m_head.m_prev = &list->m_head; // points itself
    list->m_tail.m_next = &list->m_tail; // points itself

    return list;
}


void ListDestroy(List** _pList, void (*_elementDestroy)(void* _item)){

    













}