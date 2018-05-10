#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include "stdint.h"
#include "global.h"


/*kernel链表数据结构*/
struct kernel_list{
	struct kernel_list *prev;
	struct kernel_list *next;
};

/*kernel通用队列*/
struct general_queue{
	struct kernel_list* front;
	uint32_t queue_len;
};

#define list_entry(ptr, type, member) \
        ((type*)((char*)ptr - (uint32_t)(&((type*)0)->member)))
/**
 * ptr = &(type->member)
 * type_addr = list_entry(ptr, type, member)
*/

void list_init(struct kernel_list *list);
void list_add(struct kernel_list *head, struct kernel_list *new);
void list_add_prev(struct kernel_list *head, struct kernel_list *new);
void list_del(struct kernel_list *node);
void list_del_safe(struct kernel_list *node);
uint32_t list_len(struct kernel_list *node);
bool list_find_item(struct kernel_list *list, struct kernel_list *node);


#endif