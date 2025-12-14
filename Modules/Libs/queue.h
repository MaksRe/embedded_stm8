#ifndef _QUEUE_H_
#define _QUEUE_H_

    #include "stm8s.h"

    typedef void* queue_item_t;

    typedef struct {
        void **items;
        uint16_t num_of_items;
        uint16_t item_size;

        int16_t front;
        int16_t rear;
    } cyclic_queue_t;

    bool cyclic_queue_init(cyclic_queue_t *q, uint16_t num_of_items, uint16_t item_size);
    uint8_t cyclic_queue_is_full(cyclic_queue_t *q); // Проверяем, не заполнена ли очередь
    uint8_t cyclic_queue_is_empty(cyclic_queue_t *q); // Проверяем, не пуста ли очередь
    void cyclic_queue_push(cyclic_queue_t *q, queue_item_t item); // Добавляем элемент
    queue_item_t cyclic_queue_get(cyclic_queue_t *q); // Получение очередного элемента
    queue_item_t cyclic_queue_pop(cyclic_queue_t *q); // Удаляем элемент

#endif
