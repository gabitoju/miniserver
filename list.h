#ifndef LIST
#define LIST

typedef struct _node {
    void *data;
    struct _node *next;
    struct _node *previous;

} Node;

typedef struct _list { 
    int size;
    Node *head;
    Node *tail;
} List;


List* list_create();
void list_destroy(List* list);
void list_push(List* list, void* data);
void list_remove(List* list, Node* node);

#endif // LIST
