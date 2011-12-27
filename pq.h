/**
 * @file pq.h
 *
 * @brief Implements a simple priority queue which can be locked for multiple thread access.
 */

#ifndef _PQ_H
#define _PQ_H


typedef struct {
    void* data; //>! user defined data
    uint64_t priority;
} pq_elem_t;

typedef struct {
    pq_elem_t* elems; //>! Array of elements
    int max_num; //>! Maximum capacity of the priority queue.
    int num; //>! Current size of the priority queue.
    spinlock_t lock; //>! Lock to protect the global priority queue.
} pq_t;


/** Initializes the priority queue. */
void pq_init(pq_t* pq, int max_num);

/**
 * Inserts an element into the priority queue.
 * @param pq the queue.
 * @param priority the priority value of the element.
 * @param data the element.
 */
int pq_insert(pq_t* pq, uint64_t priority, void* data);

/** Removes and returns the element with the minimum priority. */
void* pq_delete_min(pq_t* pq);

/** Returns the minimum priority value for the elements in the queue. */
uint64_t pq_get_min_priority(pq_t* pq);

/** Gets the lock of the priority queue. */
void pq_lock(pq_t* pq);

/** Releases the lock of the priority queue. */
void pq_unlock(pq_t* pq);

/** Tells whether or not the priority queue is empty. */
bool pq_empty(pq_t* pq);

#endif // _PQ_H
