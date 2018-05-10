#include "list.h"
#include "stdint.h"
#include "interrupt.h"
#include "global.h"



/**
 * list_lint --初始化链表头
 * @list: 待初始化链表头节点
*/
void list_init(struct kernel_list* list)
{
    list->next = list;
    list->prev = list;
}

/**
 * __list_add --在prev和next之间插入新节点new
*/
static inline void __list_add(struct kernel_list* new, \
                            struct kernel_list* prev, \
                            struct kernel_list* next)
{
    prev->next = new;
    new->next = next;
    next->prev = new;
    new->prev = prev;
}

/**
 * list_add --在head节点之后插入new节点
 * @head: 插入位置
 * @new: 待插入节点
*/
void list_add(struct kernel_list* head, struct kernel_list* new)
{
    __list_add(new, head, head->next);
}

/**
 * list_add_prev --在head前面插入new节点
 * @head: 插入位置
 * @new: 待插入节点
*/
void list_add_prev(struct kernel_list* head, struct kernel_list* new)
{
    __list_add(new, head->prev, head);
}

/**
 * __list_del --从链表中删除prev和next之间的节点
*/
static inline void __list_del(struct kernel_list* prev, struct kernel_list* next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * list_del --删除一个节点,删除后node节点prev和next指向NULL
 * @node: 待删除的节点
*/
void list_del(struct kernel_list* node)
{
    __list_del(node->prev, node->next);
    node->next = NULL;
    node->prev = NULL;
}

/**
 * list_del_safe --删除一个节点，删除后对node节点做初始化
 * @node: 待删除的节点
*/
void list_del_safe(struct kernel_list* node)
{
    __list_del(node->prev, node->next);
    list_init(node);
}

/**
 * list_len --返回链表长度
 * @node: 链表一个节点
*/
uint32_t list_len(struct kernel_list* node)
{
    struct kernel_list *p = node;
    uint32_t len = 0;
    while(p->next != node)
    {
        len++;
        p = p->next;
    }
    return len;
}

/**
 * list_find_item --在链表中找到元素返回TRUE，否则返回FALSE
 * @list: 待寻找的链表一个节点指针
 * @node: 待寻找的元素
*/
bool list_find_item(struct kernel_list* list, struct kernel_list* node)
{
    struct kernel_list *p = list;
    while (p->next != p)
    {
        if(p == node)
        {
            return TRUE;
        }
        p = p->next;
    }
    return FALSE;
}