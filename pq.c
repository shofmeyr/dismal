/**
 * @file pq.c
 * Implements a simple priority queue which can be locked for multiple thread access.
 */

#include <malloc.h>
#include "pq.h"

void pq_init(pq_t* pq, int max_num) {
    pq->max_num = max_num;
    pq->num = 0;
    pq->elems = calloc(max_num + 1, sizeof(pq_elem_t));

    // Position 0 is the sentinel
    pq->elems[0].priority = 0;
    pq->elems[0].data = NULL;

    spinlock_init(&pq->lock);
}

int pq_insert(pq_t* pq, uint64_t priority, void* data) {

    if (pq->num >= pq->max_num) {
        return -1;
    }

    int i = ++pq->num;
    while (pq->elems[i / 2].priority > priority) {
        pq->elems[i].priority = pq->elems[i / 2].priority;
        pq->elems[i].data = pq->elems[i / 2].data;
        i /= 2;
    }
    pq->elems[i].priority = priority;
    pq->elems[i].data = data;

    return 0;
}

void* pq_delete_min(pq_t* pq) {
    if (pq->num == 0) {
        return NULL;
    }
    void* data = pq->elems[1].data;
    pq_elem_t* last_elem = &pq->elems[pq->num--];
    int i, child = 0;
    for (i = 1; i * 2 <= pq->num; i = child) {
        child = i * 2;
        if ((child != pq->num) && (pq->elems[child + 1].priority < pq->elems[child].priority)) {
            child++;
        }
        if (last_elem->priority > pq->elems[child].priority) {
            pq->elems[i].priority = pq->elems[child].priority;
            pq->elems[i].data = pq->elems[child].data;
        }
        else {
            break;
        }
    }
    pq->elems[i].priority = last_elem->priority;
    pq->elems[i].data = last_elem->data;
    return data;
}


uint64_t pq_get_min_priority(pq_t* pq) {
    if (pq->num == 0) {
        return 0;
    }
    return pq->elems[1].priority;
}


void pq_lock(pq_t* pq) {
    spin_lock(&pq->lock);
}


void pq_unlock(pq_t* pq) {
    spin_unlock(&pq->lock);
}


bool pq_empty(pq_t* pq) {
    return (pq->num == 0);
}

