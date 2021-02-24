#if !defined(_a743365e_ee15_11e9_81b4_2a2ae2dbcce4)
#define _a743365e_ee15_11e9_81b4_2a2ae2dbcce4


typedef struct node node;
typedef node* list;
typedef struct node{
    char* data;
    list next;
};

/* list forall
*  calls act, a generic function, on each element in list l
*/
void forall(list l, void(*act)(char*s));

/* list add
*  adds a string, data, to the beginning of list l
*  data is copied by strdup before being added to the list
*/
void add(char* data, list* l);

/* list free
*  frees allocated memory associated with list l
*/
void listfree(list* l);

/* mergesort split list
*  a helper function for mergesort, splits list l into two smaller lists
*/
void splitlist(list l, list* left, list* right);

/* mergsort merge
*  a helper function for mergesort
*  recursively merges list l back together in alphabetical order
*  uses strcoll for string comparisons
*/
list merge(list left, list right);

/* list mergesort
*  outer function that sorts the given list
*  recursively splits list in half, then merges fragments back together in a sorted fashion
*/
void mergesort(list* l);


#endif //_a743365e_ee15_11e9_81b4_2a2ae2dbcce4
