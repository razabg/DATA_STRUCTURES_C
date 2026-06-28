#ifndef S_LINKED_LIST_H
#define S_LINKED_LIST_H

#include <stdio.h>

/**
 * @struct Person
 * @brief Represents a single node inside our sorted, singly linked list.
 * * This structure implements a "Pure Head" layout where the very first node
 * contains active data.
 */
typedef struct Person
{
    int m_id;              /**< The unique sorting key (Identification number) */
    struct Person *m_next; /**< Pointer referencing the next Person node in memory */
} Person;

/* ──────────────────────────────────────────────────────────────────────────
 * API FUNCTION DECLARATIONS
 * ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Inserts a new person node at the very beginning of the list.
 * * @param _head The current head pointer of the list (can be NULL).
 * @param _p    Pointer to the fully allocated person node to be inserted.
 * @return      A pointer to the new head of the list (which is now _p).
 */
Person *ListInsertHead(Person *_head, Person *_p);

/**
 * @brief Removes the first person node from the beginning of the list.
 * * @param _head The current head pointer of the list.
 * @param _item A double pointer used to output/extract the removed node's data.
 * @return      The updated head pointer of the list (pointing to the old second node).
 */
Person *ListRemoveHead(Person *_head, Person **_item);

/**
 * @brief Iteratively inserts a new person node into its correct, sorted position.
 * * Traverses the list using a loop and places the node in ascending order based on ID.
 * * @param _head The current head pointer of the list.
 * @param _key  The target ID to sort by (must match _p->m_id).
 * @param _p    Pointer to the fully allocated person node to be inserted.
 * @return      The updated head pointer of the list (unchanged unless inserted at the front).
 */
Person *ListInsertByKey(Person *_head, int _key, Person *_p);

/**
 * @brief Recursively inserts a new person node into its correct, sorted position.
 * * Leverages the execution stack to find the target position and re-link pointers asynchronously.
 * * @param _head The current head pointer of the list.
 * @param _key  The target ID to sort by (must match _p->m_id).
 * @param _p    Pointer to the fully allocated person node to be inserted.
 * @return      The updated head pointer of the list.
 */
Person *ListInsertByKeyRec(Person *_head, int _key, Person *_p);

/**
 * @brief Iteratively finds and extracts a person node from the list by their ID.
 * * Traverses the list using a loop. If found, unlinks the node and passes it back via _p.
 * * @param _head The current head pointer of the list.
 * @param _key  The unique ID key of the person to look for.
 * @param _p    A double pointer used to output/extract the removed node's data.
 * @return      The updated head pointer of the list.
 */
Person *ListRemoveByKey(Person *_head, int _key, Person **_p);

/**
 * @brief Recursively finds and extracts a person node from the list by their ID.
 * * Traverses the nodes recursively. If found, unlinks the node and passes it back via _p.
 * * @param _head The current head pointer of the list.
 * @param _key  The unique ID key of the person to look for.
 * @param _p    A double pointer used to output/extract the removed node's data.
 * @return      The updated head pointer of the list.
 */
Person *ListRemoveByKeyRec(Person *_head, int _key, Person **_p);

/**
 * @brief Traverses the entire linked list and prints out each person's data.
 * * Useful for debugging and reviewing current list states.
 * * @param _head The current head pointer of the list.
 */
void PrintList(Person *_head);

#endif /* S_LINKED_LIST_H */