/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include <stdlib.h>
#include <string.h>
#include "entry.h"
#include "data.h"


/* Função que cria uma entry, reservando a memória necessária e
 * inicializando-a com a string e o bloco de dados passados.
 */
struct entry_t *entry_create(char *key, struct data_t *data) {

  struct entry_t *entry;
  entry = (struct entry_t*) malloc(sizeof(struct entry_t));

  entry->key = key;
  entry->value = data;

  return entry;
}

/* Função que inicializa os elementos de uma entry com o
 * valor NULL.
 */
void entry_initialize(struct entry_t *entry) {

  if (entry != NULL) {
    entry->key = NULL;
    entry->value = NULL;
  }
}

/* Função que elimina uma entry, libertando a memória por ela ocupada
 */
void entry_destroy(struct entry_t *entry) {

  if (entry != NULL) {
    data_destroy(entry->value);
    free(entry->key);
    free(entry);
  }
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 */
struct entry_t *entry_dup(struct entry_t *entry) {

  if (entry == NULL) {
    return NULL;
  }
  return entry_create(strdup(entry->key), data_dup(entry->value));
}

/* Função que substitui o conteúdo de uma entrada entry_t.
*  Deve assegurar que destroi o conteúdo antigo da mesma.
*/
void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value) {

  free(entry->key);
  entry->key = new_key;
  data_destroy(entry->value);
  entry->value = new_value;
}

/* Função que compara duas entradas e retorna a ordem das mesmas.
*  Ordem das entradas é definida pela ordem das suas chaves.
*  A função devolve 0 se forem iguais, -1 se entry1<entry2, e 1 caso contrário.
*/
int entry_compare(struct entry_t *entry1, struct entry_t *entry2) {

  int result;
  char *x = entry1->key;
  char *y = entry2->key;

  if ((result = strcmp(x, y)) > 0) {
    return 1;
  } else if (result < 0) {
    return -1;
  } else {
    return result;
  }
}
