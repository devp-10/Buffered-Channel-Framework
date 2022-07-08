#include "linked_list.h"

// Creates and returns a new list
list_t* list_create() {
    list_t *list = (list_t*) malloc(sizeof(list_t));
    list->head = NULL;
    list->count = 0;
    return list;
}

// Destroys a list
void list_destroy(list_t* list) {
    while (list->count != 0) {
        list_remove(list, list->head);
    }
    free(list);
}

// Returns beginning of the list
list_node_t* list_begin(list_t* list) {
    return list->head;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node) {
    return node->next;
}

// Returns data in the given list node
void* list_data(list_node_t* node) {
    return node->data;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return 0;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data) {
    list_node_t *curr = list->head;
    while (curr != NULL) {
        if (curr->data == data)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

// Inserts a new node in the list with the given data
void list_insert(list_t* list, void* data) {
    list_node_t *node = malloc(sizeof(list_node_t));
    node->data = data;
    // if given list empty
    if (!list->head) {
        list->head = node;
        node->next = NULL;
        node->prev = NULL;
    }
    list_node_t *curr = list->head;
    list->head = node;
    node->next = curr;
    node->prev = NULL;
    curr->prev = node;
    list->count += 1;
}


// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node) {
    if (!node)
        return;
    if (!node->prev)
        list->head = node->next;
    else
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
    node->data = NULL;
    list->count -= 1;
    free(node);
}

// Executes a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data)) {
    for (list_node_t* curr=list->head; curr!=NULL; curr=curr->next) {
        (*func)(curr->data);
    }
}