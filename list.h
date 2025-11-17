#ifndef LIST
#define LIST

typedef struct Node {
    void *data;
    struct Node *next;
    struct Node *previous;

} Node;

typedef struct List { 
    int size;
    Node *head;
    Node *tail;
} List;


List* list_create();
void list_destroy(List* list);
void list_push(List* list, void* data);
void list_remove(List* list, Node* node);

#endif // LIST
