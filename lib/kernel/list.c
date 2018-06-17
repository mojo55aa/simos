/**
 * ISSUES
 * (1)链表list_find_item有bug，在长度是0的情况下已让能找到元素
 * (2)链表可能需要重构，对链表元素kernel_list进行一层封装
*/
#include "list.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "thread.h"


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
struct kernel_list* list_del(struct kernel_list* node)
{
    __list_del(node->prev, node->next);
    node->next = NULL;
    node->prev = NULL;
    return node;
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
 * list_pop --删除链表头节点的下一个节点并返回
 * @list: 链表
*/
struct kernel_list* list_pop(struct kernel_list* list)
{
    return list_del(list->next);
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
 * list_empty --判断链表是否为空
 * @list: 链表
 * return: BOOL
*/
bool list_empty(struct kernel_list* list)
{
    return (list_len(list) > 0) ? FALSE : TRUE;
}

/**
 * list_find_item --在链表中找到元素返回TRUE，否则返回FALSE
 * @list: 待寻找的链表一个节点指针
 * @node: 待寻找的元素
*/
bool list_find_item(struct kernel_list* list, struct kernel_list* node)
{
    if(list == NULL || node == NULL)
    {
        return FALSE;
    }
    struct kernel_list *p = list;
    while (p->next != list)
    {
        if(p == node)
        {
            return TRUE;
        }
        // PRINT_ADDR("item", p);
        p = p->next;
    }
    return FALSE;
}

/**
 * list_for_each --遍历链表
 * @list: 遍历起始位置
*/
void list_for_each(struct kernel_list* list)
{
    if(list == NULL)
    {
        return;
    }
    struct kernel_list *p = list;
    do
    {
        // put_hex(p);
        struct task_struct *task = list_entry(p, struct task_struct, thread_dispatch_queue);
        p = p->next;
    } while (p->next != list);
}

/**
 * queue_empty --队列是否为空
 * @queue: 队列
*/
bool queue_empty(struct general_queue* queue)
{
    ASSERT(queue != NULL);
    return (queue->queue_len == 0 ? TRUE : FALSE);
}

/**
 * queue_in --入队
 * @queue: 操作的队列
 * @ node: 入队的节点
*/
void queue_in(struct general_queue* queue, struct kernel_list* node)
{
    ASSERT(queue != NULL && node != NULL);
    // PRINT_ADDR("queue_front", queue->front);
    /*如果队列是空的，front直接指向node*/
    if(queue->queue_len == 0)
    {
        queue->front = node;
        node->next = node;
        node->prev = node;
        queue->queue_len++;
    }
    else
    {
        list_add_prev(queue->front, node);
        queue->queue_len++;
    }
    // PRINT_ADDR("queue_front after add", queue->front);
}

/**
 * queue_out --出队
 * @queue: 操作的队列
 * return: 成功返回出队的元素指针，失败返回NULL
*/
struct kernel_list* queue_out(struct general_queue* queue)
{
    ASSERT(queue != NULL);
    /*队列为空，出队失败*/
    if(queue->queue_len == 0)
    {
        return NULL;
    }

    /*如果队列还剩一个元素,出队后front指向NULL，否则指向下一个*/
    struct kernel_list *p = queue->front;
    if (queue->queue_len == 1)
    {
        queue->front = NULL;
    }
    else
    {
        queue->front = queue->front->next;
    }

    /*在队列中使第一个元素脱离*/
    list_del(p);
    queue->queue_len--;
    return p;
}

/**
 * queue_push --将元素放到队列首
 * @queue: 操作的队列
 * @node: 放入的元素
*/
void queue_push(struct general_queue* queue, struct kernel_list* node)
{
    ASSERT(queue != NULL && node != NULL);
    /*如果队列是空的，front直接指向node*/
    if(queue->queue_len == 0)
    {
        queue->front = node;
        node->next = node;
        node->prev = node;
        queue->queue_len++;
    }
    else
    {
        list_add_prev(queue->front, node);
        queue->front = node;
        queue->queue_len++;
    }
}

/**
 * queue_init --初始化一个空队列
*/
void queue_init(struct general_queue* queue)
{
    queue->front = NULL;
    queue->queue_len = 0;
}