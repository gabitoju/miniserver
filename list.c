#include <stdlib.h>
#include "list.h"

List* list_create() {
    List* list = malloc(sizeof(List));

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    return list;
}

void list_destroy(List* list) {
    if (list != NULL) {

        while (list->head != NULL) {
            Node* node = list->head;
            list->head = node->next;
            free(node);
        }

        free(list);
    }
}

void list_push(List* list, void* data) {
    Node* node = malloc(sizeof(Node));

    node->data = data;
    node->next = NULL;
    node->previous = list->tail;

    if (list->size == 0) {
        list->head = node;
    } else {
        list->tail->next = node;
    }
    list->tail = node;

    list->size++;
}

void list_remove(List* list, Node* node) {

    if (list == NULL || node == NULL) {
        return;
    }

    if (list->size > 0) {

        if (node->previous != NULL) {
            node->previous->next = node->next;
        } else {
            list->head = node->next;
        }
       
        if (node->next != NULL) {
            node->next->previous = node->previous;
        } else {
            list->tail = node->previous;
        }

        list->size--;
        free(node);
    }

}
