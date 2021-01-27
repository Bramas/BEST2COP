#ifndef __EXTENDABLE_H__
#define __EXTENDABLE_H__

#include "base.h"
#include "LabelTable.h"

typedef struct Path_s Path;


/**
 * @brief informations of an Extendable Path
 */

struct Path_s {
    my_m1 m1;       /**< represent the delay of the corresponding path */
    my_m2 m2;       /**< represent the IGP weight of the corresponding path */
};

typedef struct Extendable_s Extendable_t;

/**
 * @brief Structure which represents all the 
 * Extendables Paths
 */

struct Extendable_s {
    Path infos;
    Extendable_t* next;
};


typedef struct Extendable_list_s Extendable_list_t;

struct Extendable_list_s {
    int node;
    Extendable_t* ext;
    Extendable_list_t* next;
};


/**
 * @brief add a new Extendable Path
 * 
 * @param m1                delay of the new Path
 * @param m2                IGP weight of the new Path
 * @param next              old list of Extendables Paths
 * 
 * @return return the new list
 */

Extendable_t* Extendable_new(my_m1 m1, my_m2 m2, Extendable_t* next);


void Extendable_free(Extendable_t* ext);


/**
 * @brief only use for debug
 */

void Extendable_print(Extendable_t* ext);

/**
 * @brief check if all the element of an array of ext are NULL
 * 
 * @param ext               array to test
 * @param nbNode            array size
 * 
 * @return return true if the array is empty, false if not
 */

bool Extendable_is_empty(Extendable_t** ext, int nbNode);


/**
 * @brief copy ext into an other extendable list
 * 
 * @param ext               list to copy
 * 
 * @return return the copied list
 */

Extendable_t* Extendable_copy(Extendable_t* ext);


/**
 * @brief create a new element for a list of Extendable_list
 * 
 * @param next              old list
 * @param node              destination node of the paths
 * @param ext               all the path toward node
 * 
 * @return return the new list
 */

Extendable_list_t* Extendable_list_new(Extendable_list_t* next, int node, Extendable_t* ext);

void Extendable_list_free(Extendable_list_t* next);

#endif