/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "data.h"
#include "entry.h"
#include "tree.h"
#include "tree-private.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Serializa uma estrutura data num buffer que será alocado
 * dentro da função. Além disso, retorna o tamanho do buffer
 * alocado ou -1 em caso de erro.
 */
int data_to_buffer(struct data_t *data, char **data_buf) {
  if(data == NULL || data_buf == NULL) {
    return -1;
  }

  int size = sizeof(int) + data->datasize;

  *data_buf = malloc(size);

  memcpy(*data_buf, &data->datasize, sizeof(int));
  memcpy(*data_buf+sizeof(int), data->data, data->datasize);

  return size;
}

/* De-serializa a mensagem contida em data_buf, com tamanho
 * data_buf_size, colocando-a e retornando-a numa struct
 * data_t, cujo espaco em memoria deve ser reservado.
 * Devolve NULL em caso de erro.
 */
struct data_t *buffer_to_data(char *data_buf, int data_buf_size) {
  if(data_buf == NULL || data_buf_size < 0) {
    return NULL;
  }

  struct data_t *data;
  int datasize;

  memcpy(&datasize, data_buf, sizeof(int));
  data = data_create(datasize);

  memcpy(data->data, data_buf+sizeof(int), datasize);

  return data;
}

/* Serializa uma estrutura entry num buffer que será alocado
 * dentro da função. Além disso, retorna o tamanho deste
 * buffer ou -1 em caso de erro.
 */
int entry_to_buffer(struct entry_t *entry, char **entry_buf) {
  if(entry == NULL || entry_buf == NULL) {
    return -1;
  }

  int size = sizeof(int) + strlen(entry->key)+1 +
              sizeof(int) + entry->value->datasize;

  int length = strlen(entry->key);
  int datasize = entry->value->datasize;

  *entry_buf = malloc(size);

  memcpy(*entry_buf, &length, sizeof(int));
  memcpy(*entry_buf+sizeof(int), entry->key, length);
  memcpy(*entry_buf+sizeof(int)+length, &datasize, sizeof(int));
  memcpy(*entry_buf+2*sizeof(int)+length, entry->value->data, datasize);

  return size;
}

/* De-serializa a mensagem contida em entry_buf, com tamanho
 * entry_buf_size, colocando-a e retornando-a numa struct
 * entry_t, cujo espaco em memoria deve ser reservado.
 * Devolve NULL em caso de erro.
 */
struct entry_t *buffer_to_entry(char *entry_buf, int entry_buf_size) {
  if(entry_buf == NULL || entry_buf_size < 0) {
    return NULL;
  }

  struct entry_t *entry;
  struct data_t *data;

  int length;
  memcpy(&length, entry_buf, sizeof(int));
  char *key = malloc((length+1)*sizeof(char));
  memcpy(key, entry_buf+sizeof(int), length);
  int datasize;
  memcpy(&datasize, entry_buf+sizeof(int)+length, sizeof(int));
  data = data_create(datasize);
  memcpy(data->data, entry_buf+2*sizeof(int)+length, datasize);

  return entry_create(key, data);
}

/* Serializa uma estrutura tree num buffer que será alocado
 * dentro da função. Além disso, retorna o tamanho deste
 * buffer ou -1 em caso de erro.
 */
int tree_to_buffer(struct tree_t *tree, char **tree_buf) {
  if(tree == NULL || tree_buf == NULL ) {
    return -1;
  }

  int size = tree_node_size(tree);

  *tree_buf = malloc(size);

  tree_populate(tree, tree_buf, 0);

  return size;
}

/* De-serializa a mensagem contida em tree_buf, com tamanho
 * tree_buf_size, colocando-a e retornando-a numa struct
 * tree_t, cujo espaco em memoria deve ser reservado.
 * Devolve NULL em caso de erro.
 */
struct tree_t *buffer_to_tree(char *tree_buf, int tree_buf_size) {
  if(tree_buf == NULL || tree_buf_size < 0) {
    return NULL;
  }

  return tree_assimilate(tree_buf, 0, tree_buf_size);
}
