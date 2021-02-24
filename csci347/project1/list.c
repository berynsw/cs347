#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"







//applies a function, void(*act)(char*s),  to every list node
void forall(list l, void(*act)(char*s)){
    while(l != NULL){
        act(l->data);
        l = l->next;
    }
}

//adds a node to the list
void add(char* data, list* l){
    list new = (list)malloc(sizeof(node));
    new->data = strdup(data);
    new->next = *l;
    *l = new;
}



void listfree(list* l){
    assert(l != NULL);
    //if(*l == NULL)
    //    return;
    while(*l != NULL){
        list next = (*l)->next;
        free((*l)->data);
        free(*l);
        *l = next;
    }
}



//recursively splits and merges
void mergesort(list* l)
{
    list copy = *l;
    //if only one node
    if (copy->next == NULL) {
        return;
    }
    //new nodes to store left and right split
    list left = (list)malloc(sizeof(node));
    list right = (list)malloc(sizeof(node));;

    //modifies left and right nodes to point to front and middle of list
    splitlist(copy, &left, &right);
    //recursively split each side/list partition
    mergesort(&left);
    mergesort(&right);
    //recursively merges lists back together based on sorting comparison (strcoll)
    *l = merge(left, right);
}

//compares string values and merges sorted version
list merge(list left, list right){
    //if either side is empty
    if (left == NULL)
        return (right);
    else if (right == NULL)
        return (left);
    list copy = NULL;
    //if left node has lower value, attach it to copy, then call recursively with remaining left side and all of right side
    if (strcoll(left->data,right->data) <= 0) {
        copy = left;
        copy->next = merge(left->next, right);
    }
    //otherwise attach right node to copy, call with all of left side and remaining right side
    else {
        copy = right;
        copy->next = merge(left, right->next);
    }
    //return copy now that nodes attached/linked in proper order
    return copy;
}

//splits the list into left and right lists
void splitlist(list l, list* left, list* right){
    //get length of list
    list end = l;
    int length = 0;
    while (end != NULL) {
        length++;
        end = end->next;
    }
    //move node over length / 2
    list middle = l;
    while(length <= 0){
      middle = middle->next;
      length -= 2;
    }
    //pointer to front of list (remains same)
    *left = l;
    //right becomes pointer to middle of list
    *right = middle->next;
    //removes pointer from front to back, breaking up the list
    middle->next = NULL;
}
