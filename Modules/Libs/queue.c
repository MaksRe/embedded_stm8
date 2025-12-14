

#include "queue.h"
#include "string.h"
#include "stdlib.h"

bool cyclic_queue_init(cyclic_queue_t *q, uint16_t num_of_items, uint16_t item_size)
{
    uint8_t i = 0;

    q->items = (void **)malloc(num_of_items * item_size);
    if (q->items == NULL)
    {
        return FALSE;
    }

    for (i; i < num_of_items; i++)
    {
        q->items[i] = malloc(item_size);
        if (q->items[i] == NULL)
            return FALSE;
    }

    q->num_of_items = num_of_items;
    q->item_size = item_size;
    q->front = q->rear = -1;

    return TRUE;
}

// Проверяем, не заполнена ли очередь
uint8_t cyclic_queue_is_full(cyclic_queue_t *q) 
{
    if ((q->front == q->rear + 1) || 
        (q->front == 0 && 
         q->rear  == q->num_of_items - 1)) 
        return 1;
    return 0;
}

// Проверяем, не пуста ли очередь
uint8_t cyclic_queue_is_empty(cyclic_queue_t *q) 
{
    if (q->front == -1) 
        return 1;
    return 0;
}

// Добавляем элемент
void cyclic_queue_push(cyclic_queue_t *q, queue_item_t item) 
{
    if (cyclic_queue_is_full(q))
    {
        return;
    }
    
    if (q->front == -1) 
        q->front = 0;
    q->rear = (q->rear + 1) % q->num_of_items;

    memcpy(&q->items[q->rear], 
          (const void *)item,
          sizeof(q->item_size));
}

// Получение очередного элемента
queue_item_t cyclic_queue_get(cyclic_queue_t *q) 
{
    if (cyclic_queue_is_empty(q)) 
    {
        return (NULL);
    }
    else
    {
        queue_item_t item = q->items[q->front];

        return item;
    }
}

// Освобождение/удаление очередного элемента
queue_item_t cyclic_queue_pop(cyclic_queue_t *q) 
{
    if (cyclic_queue_is_empty(q)) 
    {
        return (NULL);
    }
    else
    {
        queue_item_t item = q->items[q->front];

        if (q->front == q->rear) 
        {
            q->front = q->rear = -1;
        }
        else 
        {
            q->front = (q->front + 1) % q->num_of_items;
        }

        return item;
    }
}
