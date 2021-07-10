/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"

struct tree_t {
  struct entry_t *entry;
  struct tree_t *left, *right;
};

/* Função auxiliar para encontrar o menor sucessor de tree
 */
struct tree_t *leftmost_child(struct tree_t *tree);

/* Função auxiliar para popular um array de char* keys com as
 * chaves de uma árvore tree
 */
int add_keys(struct tree_t *tree, char **keys, int i);

/* Função que devolve o tamanho que cada nó da árvore irá ocupar
 * na memória de um buffer de árvores
 */
int tree_node_size(struct tree_t *tree);

/* Função que armazena todos os dados serializados de todos os nós de
 * uma árvore em um array arr de memória pré-alocada
 */
int tree_populate(struct tree_t *tree, char **arr, int i);

/* Função que devolve uma tree a partir de todos os dados guardados
 * num buffer arr de tamanho arr_size
 */
struct tree_t *tree_assimilate(char *arr, int i, int arr_size);

#endif
