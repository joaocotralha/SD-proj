/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "data.h"
#include "entry.h"
#include "network_client.h"
#include "sdmessage.pb-c.h"
#include "message-private.h"
#include "zookeeper/zookeeper.h"
#include "client_stub.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ZDATALEN 1024 * 1024

/* Remote tree. A definir pelo grupo em client_stub-private.h
 */
struct rtree_t *primary;
struct rtree_t *backup;

static zhandle_t *zh;

int is_connected;

typedef struct String_vector zoo_string;

/**
* Watcher function for connection state change events
*/
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1;
		} else {
			is_connected = 0;
		}
	}
}

/**
* Data Watcher function for /MyData node
*/
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
	zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
	int zoo_data_len = ZDATALEN;
	if (state == ZOO_CONNECTED_STATE)	 {
		if (type == ZOO_CHILD_EVENT) {
      /* Get the updated children */
      if (ZOK != zoo_get_children(zh, "/kvstore", 0, children_list)) {
        perror("Error getting /kvstore children");
        return;
      }
      if(children_list->count < 1) {
        printf("No current active server\n");
        rtree_disconnect(primary);
        rtree_disconnect(backup);
        primary = NULL;
        backup = NULL;

      } else if(children_list->count < 2) {
        if(strcmp(children_list->data[0], "primary") != 0){
          rtree_disconnect(primary);
          primary = backup;
        } else {
          rtree_disconnect(backup);
        }
        backup = NULL;

      } else {
        for (int i = 0; i < children_list->count; i++) {
					char* zdata = NULL;
			    char path[20];
			    strcpy(path, "/kvstore/");

			    if(strcmp(children_list->data[i], "primary") == 0) {

			      zdata = (char *) malloc(ZDATALEN * sizeof(char));
			      if(ZOK != zoo_get(zh, strcat(path,children_list->data[i]), 0, zdata, &zoo_data_len, NULL)) {
			        perror("Could not get from zookeeper server");
			        return;
			      }

						if ((primary = rtree_connect(zdata)) == NULL){
							printf("Could not connect primary server to backup server\n");
		          return;
						}

			    } else {
			      zdata = (char *) malloc(ZDATALEN * sizeof(char));
			      if(ZOK != zoo_get(zh, strcat(path,children_list->data[i]), 0, zdata, &zoo_data_len, NULL)) {
			        perror("Could not get from zookeeper server");
			        return;
			      }

			      if ((backup = rtree_connect(zdata)) == NULL){
							printf("Could not connect primary server to backup server\n");
		          return;
						}
			    }
				}
      }

      sleep(1);
	 	  /* Get the updated children and reset the watch */
 			if (ZOK != zoo_wget_children(zh, "/kvstore", child_watcher, watcher_ctx, children_list)) {
 				printf("Error setting watch at /kvstore");
        return;
 			}
			printf("\n=== znode listing === [ /kvstore ]");
			for (int i = 0; i < children_list->count; i++)  {
				printf("\n(%d): %s", i+1, children_list->data[i]);
			}
			printf("\n=== done ===\nA aguardar comando...\n");
      free(children_list);
		 }
	 }

}

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port){
  struct rtree_t *rtree = (struct rtree_t *) malloc(sizeof(struct rtree_t));
  if(rtree == NULL)
    return NULL;

  char *addr = strtok(strdup(address_port),":");
  char *porto = strtok(NULL,"\0");

  if((rtree->descritor = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Erro ao criar socket TCP");
    return NULL;
  }

  rtree->socket.sin_family = AF_INET;
  rtree->socket.sin_port = htons(atoi(porto));


  if(inet_pton(AF_INET, addr, &rtree->socket.sin_addr) < 1){
    return NULL;
  }

  if(network_connect(rtree) < 0) {
    free(addr);
    return NULL;
  }

  rtree->server_flag = 0;

  free(addr);
  return rtree;
}

struct rtree_t *zoo_connect(char *address_port) {
  int zoo_data_len = ZDATALEN;

  primary = NULL;
  backup = NULL;

  /* Connect to ZooKeeper server */
  zh = zookeeper_init(address_port, connection_watcher,	2000, 0, NULL, 0);
  if (zh == NULL)	{
    return NULL;
  }

  static char *watcher_ctx = "ZooKeeper Data Watcher";
  zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
  if (ZOK != zoo_wget_children(zh, "/kvstore", &child_watcher, watcher_ctx, children_list)) {
    perror("Error setting watch at /kvstore");
    return NULL;
  }

  if(children_list->count < 1){
    perror("No active server at /kvstore");
    return NULL;
  }

  printf("\n=== znode listing === [ /kvstore ]");
  for (int i = 0; i < children_list->count; i++)  {

    char* zdata = NULL;
    char path[20];
    strcpy(path, "/kvstore/");

    if(strcmp(children_list->data[i], "primary") == 0) {

      zdata = (char *) malloc(ZDATALEN * sizeof(char));
      if(ZOK != zoo_get(zh, strcat(path,children_list->data[i]), 0, zdata, &zoo_data_len, NULL)) {
        perror("Could not get from zookeeper server");
        return NULL;
      }

      primary = rtree_connect(zdata);

    } else {
      zdata = (char *) malloc(ZDATALEN * sizeof(char));
      if(ZOK != zoo_get(zh, strcat(path,children_list->data[i]), 0, zdata, &zoo_data_len, NULL)) {
        perror("Could not get from zookeeper server");
        return NULL;
      }

      backup = rtree_connect(zdata);
    }
    printf("\n(%d): %s", i+1, children_list->data[i]);
  }
  printf("\n=== done ===\n");

  return primary;
}


/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect(struct rtree_t *rtree){
  if(rtree == NULL) {
    return -1;
  }
	if (network_close(rtree) == -1) {
    return -1;
  }
	free(rtree);
	return 0;
}

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct rtree_t *rtree, struct entry_t *entry){
  struct message_t *msg_send, *msg_recv;

  if(rtree->server_flag) {
    primary = rtree;
  } else if(primary == NULL || backup == NULL){
    printf("At least one server is not online.\n");
    return -1;
  }

  if (rtree == NULL) {
    return -1;
  }

   msg_send = (struct message_t *) malloc(sizeof(struct message_t));
   if(msg_send == NULL)
     return -1;

   message_init(msg_send);
   msg_send->bmessage_t.opcode = 40;
   msg_send->bmessage_t.c_type = 30;
   msg_send->bmessage_t.key = strdup(entry->key);
   msg_send->bmessage_t.data_size = entry->value->datasize;
   msg_send->bmessage_t.data = entry->value->data;

   if(msg_send->bmessage_t.key == NULL || msg_send->bmessage_t.data == NULL){
     free(msg_send->bmessage_t.key);
     free(msg_send->message_t);
     free(msg_send);
     return -1;
   }

   if((msg_recv = network_send_receive(primary, msg_send)) == NULL) {
     perror("network send and receive failure");
     free(msg_send->bmessage_t.key);
     free(msg_send->message_t);
     free(msg_send);
     return -1;
   }

   int res = -1;
   if((msg_recv->message_t->opcode == 40+1)
      && (msg_recv->message_t->c_type == 60))
      res = msg_recv->message_t->op_n;

    free(msg_send->bmessage_t.key);
    free(msg_send->message_t);
    free(msg_send);
    free(msg_recv->message_t->key);
    free(msg_recv->message_t->data);
    free(msg_recv->message_t);
    free(msg_recv);
    return res;
}

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(struct rtree_t *rtree, char *key){
  struct message_t *msg_send, *msg_recv;

  if(primary == NULL || backup == NULL){
    printf("At least one server is not online.\n");
    return NULL;
  }

  if(rtree == NULL)
    return NULL;


  msg_send = (struct message_t *) malloc(sizeof(struct message_t));
  if(msg_send == NULL)
    return NULL;

  message_init(msg_send);
  msg_send->bmessage_t.opcode = 30;
  msg_send->bmessage_t.c_type = 10;
  msg_send->bmessage_t.key = strdup(key);

  if(msg_send->bmessage_t.key == NULL){
    free(msg_send);
    return NULL;
  }

  if((msg_recv = network_send_receive(backup, msg_send)) == NULL) {
    perror("network send and receive failure");
    free(msg_send->bmessage_t.key);
    free(msg_send->message_t);
    free(msg_send);
    return NULL;
  }

  struct data_t *res = NULL;
  if((msg_recv->message_t->opcode == 30+1)
      && (msg_recv->message_t->c_type == 20))
    res = data_create2(msg_recv->message_t->data_size,
                        msg_recv->message_t->data);


  free(msg_send->bmessage_t.key);
  free(msg_send->message_t);
  free(msg_send);
  free(msg_recv->message_t->key);
  free(msg_recv->message_t);
  free(msg_recv);
  return res;
}

/* Função para remover um elemento da árvore. Vai libertar
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(struct rtree_t *rtree, char *key){
  struct message_t *msg_send, *msg_recv;

  if(rtree->server_flag) {
    primary = rtree;
  } else if(primary == NULL || backup == NULL){
    printf("At least one server is not online.\n");
    return -1;
  }

  if(rtree == NULL){
    return -1;
  }

  msg_send = (struct message_t *) malloc(sizeof(struct message_t));
  if(msg_send == NULL) {
    return -1;
  }
  message_init(msg_send);
  msg_send->bmessage_t.opcode = 20;
  msg_send->bmessage_t.c_type = 10;
  msg_send->bmessage_t.key = key;

  if((msg_recv = network_send_receive(primary, msg_send)) == NULL) {
    perror("network send and receive failure");
    free(msg_send->message_t);
    free(msg_send);
    return -1;
  }

  int res = -1;
  if((msg_recv->message_t->opcode == 20+1)
      && (msg_recv->message_t->c_type == 50))
    res = msg_recv->message_t->op_n;


  free(msg_send->message_t);
  free(msg_send);
  free(msg_recv->message_t->key);
  free(msg_recv->message_t);
  free(msg_recv);
  return res;
}

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size(struct rtree_t *rtree){
  struct message_t *msg_send, *msg_recv;

  if(primary == NULL || backup == NULL){
    printf("At least one server is not online.\n");
    return -1;
  }

  if(rtree == NULL)
    return -1;


  msg_send = (struct message_t *) malloc(sizeof(struct message_t));
  if(msg_send == NULL)
    return -1;

  message_init(msg_send);
  msg_send->bmessage_t.opcode = 10;
  msg_send->bmessage_t.c_type = 60;
  if((msg_recv = network_send_receive(backup, msg_send)) == NULL) {
    perror("network send and receive failure");
    free(msg_send->message_t);
    free(msg_send);
    return -1;
  }

  int res = -1;
  if((msg_recv->message_t->opcode == 10+1)
    && (msg_recv->message_t->c_type == 50))
    res = msg_recv->message_t->result;


  free(msg_send->message_t);
  free(msg_send);
  free(msg_recv->message_t);
  free(msg_recv);
  return res;
}

/* Função que devolve a altura da árvore.
 */
int rtree_height(struct rtree_t *rtree){
  struct message_t *msg_send, *msg_recv;

  if(primary == NULL || backup == NULL){
    printf("At least one server is not online.\n");
    return -1;
  }

  if(rtree == NULL){
    return -1;
  }

  msg_send = (struct message_t *) malloc(sizeof(struct message_t));
  if(msg_send == NULL) {
    return -1;
  }

  message_init(msg_send);
  msg_send->bmessage_t.opcode = 60;
  msg_send->bmessage_t.c_type = 60;

  if((msg_recv = network_send_receive(backup, msg_send)) == NULL) {
    perror("network send and receive failure");
    free(msg_send->message_t);
    free(msg_send);
    return -1;
  }

  int res = -1;
  if((msg_recv->message_t->opcode == 60+1)
      && (msg_recv->message_t->c_type == 50))
    res = msg_recv->message_t->result;

  free(msg_send->message_t);
  free(msg_send);
  free(msg_recv->message_t);
  free(msg_recv);
  return res;
}

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys(struct rtree_t *rtree){
  struct message_t *msg_send, *msg_recv;

  if(primary == NULL || backup == NULL){
    printf("At least one server is not online.\n");
    return NULL;
  }

  if(rtree == NULL)
    return NULL;


  msg_send = (struct message_t *) malloc(sizeof(struct message_t));
  if(msg_send == NULL)
    return NULL;

  message_init(msg_send);
  msg_send->bmessage_t.opcode = 50;
  msg_send->bmessage_t.c_type = 60;

  if((msg_recv = network_send_receive(backup, msg_send)) == NULL) {
    free(msg_send->message_t);
    free(msg_send);
    return NULL;
  }

  if((msg_recv->message_t->opcode != 50+1) ||
      (msg_recv->message_t->c_type != 40) || msg_recv->message_t->n_keys == 0){
    free(msg_send->message_t);
    free(msg_send);
    free(msg_recv->message_t->keys);
    free(msg_recv->message_t);
    free(msg_recv);
    return NULL;
  }

  char **keys = malloc(sizeof(char *)*msg_recv->message_t->n_keys+1);

  for(int i=0; i<msg_recv->message_t->n_keys; i++){
    keys[i] = strdup(msg_recv->message_t->keys[i]);
    free(msg_recv->message_t->keys[i]);
  }

  if(strcmp(keys[0],"\0") == 0){
    free(keys[0]);
    free(keys);
    keys = NULL;
  }

  free(msg_send->message_t);
  free(msg_send);
  free(msg_recv->message_t->keys);
  free(msg_recv->message_t);
  free(msg_recv);
  return keys;
}

/* Liberta a memória alocada por rtree_get_keys().
 */
void rtree_free_keys(char **keys){
  int i = 0;
  while(strcmp(keys[i],"\0") != 0){
    free(keys[i]);
    i++;
  }
  free(keys[i]);
  free(keys);
}

/* Verifica se a operação identificada por op_n foi executada.
 * devolve 0 se nao foi executada, caso contrario 1 ou -1 em erro
 */
int rtree_verify(struct rtree_t *rtree,int op_n){
  struct message_t *msg_send, *msg_recv;

  if(primary == NULL || backup == NULL){
    printf("At least one server is not online.\n");
    return -1;
  }

  if(rtree == NULL)
    return -1;


  msg_send = (struct message_t *) malloc(sizeof(struct message_t));
  if(msg_send == NULL)
    return -1;


  message_init(msg_send);
  msg_send->bmessage_t.opcode = 70;
  msg_send->bmessage_t.c_type = 50;
  msg_send->bmessage_t.op_n = op_n;

  if((msg_recv = network_send_receive(backup, msg_send)) == NULL) {
    free(msg_send);
    return -1;
  }

  int res = -1;
  if((msg_recv->message_t->opcode == 70+1)
      && (msg_recv->message_t->c_type == 50))
    res = msg_recv->message_t->result;

  free(msg_send->message_t);
  free(msg_send);
  free(msg_recv->message_t);
  free(msg_recv);
  return res;
}
