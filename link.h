#ifndef LINK_H
#define LINK_H

typedef struct s_linked {
    struct s_linked *next;
    void *p;
} s_linked_t;

int
length(s_linked_t *l);

void *
at(s_linked_t *l, int i);

s_linked_t *
push(s_linked_t *l, void *push);

s_linked_t *
pop(s_linked_t *l, void **pop);

s_linked_t *
append(s_linked_t *l, void *append);

void *
reduce(s_linked_t *l, void *arg, void *f(void *val, void *arg));

void 
foreach(s_linked_t *l, void *arg, void f(void *val, void *arg));

#endif
