#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include "list.h"

int find_max(list l){
    int max = 0;
    while(l != NULL){
        if(strlen(l->data) > max)
            max = strlen(l->data);
        l = l->next;
    }
    return max;
}

int listlen(list l){
    int count = 0;
    while(l != NULL){
        count++;
        l = l->next;
    }
    return count;
}

void forall(list l, void(*act)(char*s)){
    while(l != NULL){
        act(l->data);
        l = l->next;
    }
}

void add(char* data, list* l){
    list newnode = (list)malloc(sizeof(node));
    newnode->data = strdup(data);
    newnode->next = NULL;
    while(*l != NULL)
        l = &(*l)->next;
    *l = newnode;
}

void listfree(list* l){
    assert(l != NULL);
    while(*l != NULL){
        list next = (*l)->next;
        free((*l)->data);
        free(*l);
        *l = next;
    }
}



void merge_sort_list(list* l)
{
    list copy = *l;
    if (copy != NULL && copy->next != NULL){
        list left = NULL;
        list right = NULL;
        splitlist(copy, &left, &right);

        merge_sort_list(&left);
        merge_sort_list(&right);

        *l = merge(left, right);
    }
}


list merge(list left, list right){
    if (left == NULL)
        return (right);
    else if (right == NULL)
        return (left);
    else{
        list copy = NULL;
        if (strcoll(left->data,right->data) <= 0) {
            copy = left;
            copy->next = merge(left->next, right);
        }
        else {
            copy = right;
            copy->next = merge(left, right->next);
        }
        return copy;
    }
}

void splitlist(list l, list* left, list* right){
    list end = l;
    int length = 0;
    while (end != NULL) {
        length++;
        end = end->next;
    }
    list middle = l;
    while(length <= 0){
      middle = middle->next;
      length -= 2;
    }
    *left = l;
    *right = middle->next;
    middle->next = NULL;
}
