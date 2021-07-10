/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Função que cria um novo elemento de dados data_t e reserva a memória
 * necessária, especificada pelo parâmetro size
 */
struct data_t *data_create(int size) {
    if(size <= 0){
      return NULL;
    }

    struct data_t *ret;
    ret = (struct data_t *) malloc(sizeof(struct data_t));

    ret->datasize = size;
    ret->data  = malloc(size);

    return ret;
}

/* Função idêntica à anterior, mas que inicializa os dados de acordo com
 * o parâmetro data.
 */
struct data_t *data_create2(int size, void *data) {
    if(data == NULL){
      return NULL;
    }

    struct data_t *ret;
    if((ret = data_create(size)) == NULL) {
      return NULL;
    }

    ret->datasize = size;
    free(ret->data);
    ret->data = data;

    return ret;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 */
void data_destroy(struct data_t *data) {
    if (data != NULL) {
      free(data->data);
      free(data);
    }
}

/* Função que duplica uma estrutura data_t, reservando a memória
 * necessária para a nova estrutura.
 */
struct data_t *data_dup(struct data_t *data) {
    if(data != NULL && data->datasize > 0 && data->data != NULL) {
      void *new_data;
      if((new_data = malloc(sizeof(char)*(data->datasize+1))) == NULL){
        return NULL;
      }
      memcpy(new_data, data->data, data->datasize+1);
      return data_create2(data->datasize, new_data);
    }
    return NULL;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
*  Deve assegurar que destroi o conteúdo antigo do mesmo.
*/
void data_replace(struct data_t *data, int new_size, void *new_data) {
    data->datasize = new_size;
    free(data->data);
    data->data = new_data;
}
