/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "data.h"
#include "entry.h"
#include "tree-private.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


struct tree_t; /* A definir pelo grupo em tree-private.h */
/* Função para criar uma nova árvore tree vazia.
 * Em caso de erro retorna NULL.
 */
struct tree_t *tree_create() {
  struct tree_t *new_tree = (struct tree_t*) malloc(sizeof(struct tree_t));

  new_tree->entry = NULL;
  new_tree->left = NULL;
  new_tree->right = NULL;

  if(new_tree == NULL) {
    return NULL;
  }

  return new_tree;
}

/* Função para libertar toda a memória ocupada por uma árvore.
 */
void tree_destroy(struct tree_t *tree) {
  if(tree != NULL) {
    tree_destroy(tree->left);
    tree_destroy(tree->right);
    entry_destroy(tree->entry);
    free(tree);
  }
}

/* Função para adicionar um par chave-valor à árvore.
 * Os dados de entrada desta função deverão ser copiados, ou seja, a
 * função vai *COPIAR* a key (string) e os dados para um novo espaço de
 * memória que tem de ser reservado. Se a key já existir na árvore,
 * a função tem de substituir a entrada existente pela nova, fazendo
 * a necessária gestão da memória para armazenar os novos dados.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int tree_put(struct tree_t *tree, char *key, struct data_t *value) {
  if (tree == NULL) {
    return -1;
  }

  struct entry_t *new_entry = entry_create(strdup(key), data_dup(value));

  if (tree->entry == NULL) {
    tree->entry = new_entry;
    return 0;
  }

  int res = entry_compare(tree->entry, new_entry);

  if (res > 0) {
    if(tree->left == NULL) {
      struct tree_t *new_tree = tree_create();
      new_tree->entry = new_entry;
      tree->left = new_tree;

    } else {
      tree_put(tree->left, key, value);
      entry_destroy(new_entry);
    }

  } else if (res < 0) {
    if(tree->right == NULL) {
      struct tree_t *new_tree = tree_create();
      new_tree->entry = new_entry;
      tree->right = new_tree;

    } else {
      tree_put(tree->right, key, value);
      entry_destroy(new_entry);
    }

  } else {
    entry_replace(tree->entry, new_entry->key, new_entry->value);
    free(new_entry);
  }

  return 0;
}

/* Função para obter da árvore o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser
 * libertados no contexto da função que chamou tree_get, ou seja, a
 * função aloca memória para armazenar uma *CÓPIA* dos dados da árvore,
 * retorna o endereço desta memória com a cópia dos dados, assumindo-se
 * que esta memória será depois libertada pelo programa que chamou
 * a função.
 * Devolve NULL em caso de erro.
 */
struct data_t *tree_get(struct tree_t *tree, char *key) {
  if (tree == NULL || tree->entry == NULL) {
    return NULL;
  }

  struct data_t  *ret_data;
  struct entry_t *new_entry = entry_create(strdup(key), NULL);

  int res = entry_compare(tree->entry, new_entry);
  entry_destroy(new_entry);

  if (res > 0) {
    ret_data = tree_get(tree->left, key);
  } else if (res < 0) {
    ret_data = tree_get(tree->right, key);
  } else {
    ret_data = data_dup((tree->entry)->value);
  }

  return ret_data;
}

/* Função para remover um elemento da árvore, indicado pela chave key,
 * libertando toda a memória alocada na respetiva operação tree_put.
 * Retorna 0 (ok) ou -1 (key not found).
 */
int tree_del(struct tree_t *tree, char *key) {
  if (tree == NULL || tree->entry == NULL) {
    return -1;
  }

  struct entry_t *new_entry = entry_create(strdup(key), NULL);
  int res = entry_compare(tree->entry, new_entry);
  entry_destroy(new_entry);

  if (res > 0) {
    return tree_del(tree->left, key);
  } else if (res < 0) {
    return tree_del(tree->right, key);
  } else {
    if(tree->left == NULL && tree->right == NULL) {
      entry_destroy(tree->entry);
      tree->entry = NULL;
      tree = NULL;
      return 0;

    } else if(tree->left == NULL) {
      struct tree_t *temp;
      temp = tree->right;

      free(tree->entry);
      tree->entry = temp->entry;
      tree->left = temp->left;
      tree->right = temp->right;
      free(temp);
      return 0;

    } else if(tree->right == NULL) {
      struct tree_t *temp;
      temp = tree->left;

      free(tree->entry);
      tree->entry = temp->entry;
      tree->left = temp->left;
      tree->right = temp->right;
      free(temp);
      return 0;

    }
    struct tree_t *temp = leftmost_child(tree->right);
    entry_replace(tree->entry, (temp->entry)->key, (temp->entry)->value);
    tree_del(tree->right, (tree->entry)->key);
    return 0;
  }

}


/* Função que devolve o número de elementos contidos na árvore.
 */
int tree_size(struct tree_t *tree) {
  if (tree == NULL || tree->entry == NULL) {
    return 0;
  }

  return 1 + tree_size(tree->left) + tree_size(tree->right);
}

/* Função que devolve a altura da árvore.
 */
int tree_height(struct tree_t *tree) {
  if(tree == NULL || tree->entry == NULL) {
    return 0;
  }

  int left = 1 + tree_height(tree->left);
  int right = 1 + tree_height(tree->right);
  if (left < right) {
    return right;
  } else {
    return left;
  }

}

/* Função que devolve um array de char* com a cópia de todas as keys da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
char **tree_get_keys(struct tree_t *tree) {

  int size = tree_size(tree);
  char **keys = malloc((size + 1) * sizeof(char*));

  if(size > 1) {
    add_keys(tree, keys, 0);
  } else if(size > 0) {
    keys[0] = strdup((tree->entry)->key);
  }

  keys[size] = "\0";
  return keys;
}

/* Função que liberta toda a memória alocada por tree_get_keys().
 */
void tree_free_keys(char **keys) {
  for(int i = 0; keys[i] != NULL; i++) {
    free(keys[i]);
  }
  free(keys);
}

/* Função auxiliar para encontrar o menor sucessor de tree
 */
struct tree_t *leftmost_child(struct tree_t *tree) {
  if (tree->left == NULL) {
    return tree;
  }

  return leftmost_child(tree->left);
}

/* Função auxiliar para popular um array de char* keys com as
 * chaves de uma árvore tree
 */
int add_keys(struct tree_t *tree, char **keys, int i) {
  if(tree == NULL) {
    return i;
  }

  keys[i] = strdup((tree->entry)->key);
  i++;

  i = add_keys(tree->left, keys, i);
  i = add_keys(tree->right, keys, i);

  return i;
}

/* Função que devolve o tamanho que cada nó da árvore irá ocupar
 * na memória de um buffer de árvores
 */
int tree_node_size(struct tree_t *tree) {
  if (tree == NULL) {
    return 0;
  }

  int res = sizeof(int) + strlen(tree->entry->key)+1 +
          sizeof(int) + tree->entry->value->datasize;

  return res + tree_node_size(tree->left) + tree_node_size(tree->right);
}

/* Função que armazena todos os dados de todos os nós de uma árvore
 * em um array arr de memória pré-alocada
 */
int tree_populate(struct tree_t *tree, char **arr, int i) {
  if(tree == NULL) {
    return i;
  }

  int length = strlen(tree->entry->key)+1;
  int datasize = tree->entry->value->datasize;

  memcpy(*arr+i, &length, sizeof(int));
  memcpy(*arr+i+sizeof(int), tree->entry->key, length);
  memcpy(*arr+i+sizeof(int)+length, &datasize, sizeof(int));
  memcpy(*arr+i+2*sizeof(int)+length, tree->entry->value->data, datasize);

  i += sizeof(int) + strlen(tree->entry->key)+1 +
        sizeof(int) + tree->entry->value->datasize;

  i = tree_populate(tree->left, arr, i);
  i = tree_populate(tree->right, arr, i);

  return i;
}

/* Função que devolve uma tree a partir de todos os dados guardados
 * num buffer arr
 */
struct tree_t *tree_assimilate(char *arr, int i , int arr_size) {
  if(i >= arr_size) {
    return NULL;
  }

  struct tree_t *tree;
  struct entry_t *entry;
  struct data_t *data;

  int length;
  memcpy(&length, arr+i, sizeof(int));
  char *key = (char *) malloc((length+1)*sizeof(char));
  memcpy(key, arr+i+sizeof(int), length);
  int datasize;
  memcpy(&datasize, arr+i+sizeof(int)+length, sizeof(int));
  data = data_create(datasize);
  memcpy(data->data, arr+i+2*sizeof(int)+length, datasize);

  tree = tree_create();
  tree->entry = entry_create(key, data);

  if(key == NULL) {
    return tree;
  }

  i += sizeof(int) + strlen(tree->entry->key)+1 +
        sizeof(int) + tree->entry->value->datasize;

  tree->left  = tree_assimilate(arr, i, arr_size);
  tree->right = tree_assimilate(arr, i+tree_node_size(tree->left), arr_size);

  return tree;
}
