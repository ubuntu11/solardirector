
#ifndef __LIST_H
#define __LIST_H

#include <time.h>

struct _llist;
typedef struct _llist * list;

#ifdef __cplusplus
extern "C" {
#endif
/* Define the list functions */
list list_create( void );
int list_destroy( list );
list list_dup( list );
void *list_add( list, void *, int );
int list_add_list( list, list );
int list_delete( list, void * );
void *list_get_next( list );
#define list_next list_get_next
int list_reset( list );
int list_count( list );
int list_is_next( list );
//typedef int (*list_compare)(list_item, list_item);
typedef int (*list_compare)(void *, void *);
int list_sort( list, list_compare, int);
time_t list_updated(list);
#ifdef __cplusplus
}
#endif

#endif /* __LIST_H */
