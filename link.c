#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>

#include "link.h"

int
length(s_linked_t *l)
{
    if(l == (s_linked_t *)NULL)
        return 0;

    return 1 + length(l->next);
}

void *
at(s_linked_t *l, int i)
{
    if(l == (s_linked_t *)NULL || i < 0)
        return (s_linked_t *)NULL;

    if(i == 0) return l->p;

    return at(l->next, i--);
}

s_linked_t *
push(s_linked_t *l, void *push)
{
    if(push){
        s_linked_t *n = (s_linked_t *)calloc(1,sizeof(s_linked_t));
        if(n != (s_linked_t *)NULL){
            n->p = push;
            n->next = l;
            return n;
        }
    }
    return l;
}

s_linked_t *
pop(s_linked_t *l, void **pop)
{
    *pop = (void *)NULL;
    if(l){
        s_linked_t *n = l->next;
        if (pop != (void **)NULL)
            *pop = l->p;
        free(l);
        return n;    
    }
    return l;
}

s_linked_t *
append(s_linked_t *l, void *append)
{
    s_linked_t *a, *t = l;

    if(t)
        while(t->next != (s_linked_t *)NULL)
            t = t->next;

    a = (s_linked_t *)calloc(1,sizeof(s_linked_t));
    if(a != (s_linked_t *)NULL){
        a->p = append; 
        
        if (t)
            t->next = a; 

        return l ? l : a;
    }
    return l;
}

void *
reduce(s_linked_t *l, void *arg, void *f(void *val, void *arg))
{
   while(l != (s_linked_t *)NULL){
        void *p;
        l = pop(l, &p);
        arg = f(p, arg);
        free(p);
    }
    return arg;
}

void 
foreach(s_linked_t *l, void *arg, void f(void *val, void *arg))
{
   while(l != (s_linked_t *)NULL){
        f(l->p, arg);
        l = l->next;
    }
}

#ifdef TEST_LINK
void *
print_element(void *p, void *arg)
{
    if(p) printf(" %i", *((int *)p));
    return (void *)NULL;
}

void *
add_element(void *p, void *arg)
{
    return (void *)(*(int*)p + (int)arg);
}

int main(int argc, char *argv[])
{
    int c;
    s_linked_t *l = (s_linked_t *)NULL;
    int i = atoi(argv[argc - 1]);

    for(; i > 0; i--)
    {
        int *j = (int *)malloc(sizeof(int));
        *j = 1;
        l = push(l, j);
    }

    c = (int)reduce(l, (void *)10, add_element);

    if(c != atoi(argv[argc - 1]) + 10){
        printf("Reduce count %i is not equal to %i\n", c, atoi(argv[argc - 1]) + 10);
        return EXIT_FAILURE; 
    }

    return EXIT_SUCCESS;
}
#endif
