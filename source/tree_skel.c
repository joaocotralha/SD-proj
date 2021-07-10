/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "sdmessage.pb-c.h"
#include "message-private.h"
#include "tree_skel.h"
#include "tree.h"
#include "entry.h"
#include "zookeeper/zookeeper.h"
#include "client_stub.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define ZDATALEN 1024 * 1024

pthread_mutex_t queue_lock, tree_lock;
pthread_cond_t queue_not_empty;

struct task_t *queue_head;
struct tree_t *tree;

int last_assigned;
int op_count;
int is_connected;

static zhandle_t *zh;

static char *curr_node;
static char *watcher_ctx = "ZooKeeper Data Watcher";

struct rtree_t *other_server;

typedef struct String_vector zoo_string;

/* Função do thread secundário que vai processar pedidos de escrita.*/
void *process_task (void *params){

  printf("*Thread de processamento criada*\n");

  struct thread_parameters *tp = (struct thread_parameters *) params;

  while(1){
    struct task_t *task;

    pthread_mutex_lock(&queue_lock);
    while(queue_head == NULL)
      pthread_cond_wait(&queue_not_empty, &queue_lock);

    task = queue_head;
    queue_head = task->next;
    pthread_mutex_unlock(&queue_lock);

    pthread_mutex_lock(&tree_lock);
    if(task->op == 0) { //OP_DEL
      tree_del(tree, task->key);

      if(strcmp(curr_node, "primary") == 0) {
        sleep(1);
        if(rtree_del(other_server, task->key) < 0)
          printf("There was a problem sending the operation to backup server\n");
        printf("Servidor à espera de ligações:\n");
      }

    } else { //OP_PUT
      tree_put(tree, task->key, task->data);

      if(strcmp(curr_node, "primary") == 0) {
        sleep(1);
        struct entry_t *entry = entry_create(task->key, task->data);
        if(rtree_put(other_server, entry) < 0)
          printf("There was a problem sending the operation to backup server\n");
        printf("Servidor à espera de ligações:\n");
      }
    }

    op_count++;
    pthread_mutex_unlock(&tree_lock);

  }

  printf("*Thread de processamento terminada*\n");
}

/**
* Watcher function for connection state change events
*/
void server_conn_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1;
		} else {
			is_connected = 0;
		}
	}
}

/**
* Data Watcher function for /kvstore node
*/
static void server_child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
	zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
	int zoo_data_len = ZDATALEN;
	if (state == ZOO_CONNECTED_STATE)	 {
		if (type == ZOO_CHILD_EVENT) {
	 	   /* Get the updated children */
 			if (ZOK != zoo_get_children(zh, "/kvstore", 0, children_list)) {
 				perror("Error getting /kvstore children");
        return;
 			}

      //count < 1
			if(children_list->count < 2) {
        if(strcmp(curr_node,"primary") != 0){
          int zoo_data_len = ZDATALEN;
          char* zdata = NULL;
          zdata = (char *) malloc(ZDATALEN * sizeof(char));
          if(ZOK != zoo_get(zh, "/kvstore/backup", 0, zdata, &zoo_data_len, NULL)){
            perror("Could not get from zookeeper server");
            free(zdata);
            return;
          }

          if (ZOK != zoo_delete(zh, "/kvstore/backup", -1)) {
               perror("Could not delete zookeeper node");
               return;
    			}
          if (ZOK != zoo_create(zh, "/kvstore/primary", zdata, 20, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0)) {
               perror("Could not create zookeeper node");
               return;
    			}
          curr_node = "primary";
        }
        other_server = NULL;
      } else {
        for (int i = 0; i < children_list->count; i++)  {
          if((strcmp(curr_node, children_list->data[i]) != 0) &&
            (strcmp(curr_node, "primary") == 0))  {
            char* zdata = NULL;
            char path[20];
            strcpy(path, "/kvstore/");
            zdata = (char *) malloc(ZDATALEN * sizeof(char));
            if(ZOK != zoo_get(zh, strcat(path,children_list->data[i]), 0, zdata, &zoo_data_len, NULL)){
              perror("Could not get from zookeeper server");
              free(zdata);
              return;
            }

            if((other_server = rtree_connect(zdata)) == NULL){
              printf("Could not connect primary server to backup server\n");
              return;
            }
            other_server->server_flag = 1;
          }
  			}

      }

      /* Get the updated children and reset the watch */
      if (ZOK != zoo_wget_children(zh, "/kvstore", server_child_watcher, watcher_ctx, children_list)) {
 				perror("Error setting watch at /kvstore");
        return;
 			}
      printf("\n=== znode listing === [ /kvstore ]");
      for (int i = 0; i < children_list->count; i++)  {
        printf("\n(%d): %s", i+1, children_list->data[i]);
      }
			printf("\n=== done ===\n");
      printf("CURRENT:%s\n", curr_node);
		 }
	 }
	 free(children_list);
}

int tree_zoo(char* server_port, char* zoo_port) {
  int zoo_data_len = ZDATALEN;
  other_server = NULL;
  zoo_string* children_list =	NULL;

  /* Connect to ZooKeeper server */
	zh = zookeeper_init(zoo_port, server_conn_watcher,	2000, 0, NULL, 0);
	if (zh == NULL)	{
    perror("Could not connect to zookeeper server");
		return -1;
	}

  char *addr = strtok(strdup(zoo_port),":");
  addr = strcat(addr, ":");
  addr = strcat(addr, server_port);

  sleep(3);

  if (is_connected) {

    if (ZNONODE == zoo_exists(zh, "/kvstore", 0, NULL)) {
      if (ZOK != zoo_create(zh, "/kvstore", zoo_port, 6, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL, 0)) {
           printf("Could not create zookeeper node\n");
           return -1;
			}
      if (ZOK != zoo_create(zh, "/kvstore/primary", addr, 20, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0)) {
           printf("Could not create zookeeper node\n");
           return -1;
			}
      curr_node = "primary";

    } else {
      if(ZNONODE == zoo_exists(zh, "/kvstore/primary", 0, NULL)) {
        if (ZOK != zoo_create(zh, "/kvstore/primary", addr, 20, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0)) {
          printf("Could not create zookeeper node\n");
          return -1;
        }
        curr_node = "primary";

      } else if(ZNONODE == zoo_exists(zh, "/kvstore/backup", 0, NULL)) {
        if (ZOK != zoo_create(zh, "/kvstore/backup", addr, 20, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0)) {
          printf("Could not create zookeeper node\n");
          return -1;
        }
        curr_node = "backup";
      } else {
        printf("Both servers already online\n");
        return -1;
      }
    }
  }

  children_list =	(zoo_string *) malloc(sizeof(zoo_string));
  if (ZOK != zoo_wget_children(zh, "/kvstore", &server_child_watcher, watcher_ctx, children_list)) {
    printf("Error setting watch at /kvstore");
  }

  printf("\n=== znode listing === [ /kvstore ]");
  if(children_list->count > 1) {
    for (int i = 0; i < children_list->count; i++)  {
      if((strcmp(curr_node, children_list->data[i]) != 0) &&
        (strcmp(curr_node, "primary") == 0))  {
        char* zdata = NULL;
        char path[20];
        strcpy(path, "/kvstore/");
        zdata = (char *)malloc(ZDATALEN * sizeof(char));
        if(ZOK != zoo_get(zh, strcat(path,children_list->data[i]), 0, zdata, &zoo_data_len, NULL)){
          printf("Could not get from zookeeper server\n");
          free(zdata);
          return -1;
        }
        if((other_server = rtree_connect(zdata)) == NULL){
          printf("Could not connect primary server to backup server\n");
          return -1;
        }
        other_server->server_flag = 1;
      }
      printf("\n(%d): %s", i+1, children_list->data[i]);
    }

  } else {
    other_server = NULL;
    printf("\n(%d): %s", 1, children_list->data[0]);
  }
  printf("\n=== done ===\n");
  printf("CURRENT: %s\n", curr_node);

  return 0;
}

/* Inicia o skeleton da árvore.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int tree_skel_init(){
  if((tree = tree_create()) == NULL){
    perror("failure creating tree");
    return -1;
  }

  pthread_t queue_thread;

  if (pthread_create(&queue_thread, NULL, &process_task, NULL) != 0){
		perror("failure creating process thread");
		return -1;
	}

  last_assigned = 0;
  op_count = 0;

  return 0;
}

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy(){
  tree_destroy(tree);
}

/* Verifica se a operação identificada por op_n foi executada.
 * devolve 0 se nao foi executada e caso contrario 1, ou -1 em erro
 */
int verify(int op_n){
  if (op_n < 0)
    return -1;

  int res = 0;
  if(op_n > last_assigned-1)
    return res;
  if(op_n <= op_count)
    res = 1;
  return res;
}

void queue_add_task (struct task_t *task){
  pthread_mutex_lock(&queue_lock);
  if(queue_head == NULL) {
    queue_head = task;
    task->next = NULL;
  } else {
    struct task_t *temp = queue_head;
    while(temp->next != NULL)
      temp = temp->next;
    temp->next = task;
    task->next = NULL;
  }
  pthread_cond_signal(&queue_not_empty);
  pthread_mutex_unlock(&queue_lock);
}

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(struct message_t *msg){
  struct task_t *task;

  int res;
  int op_n;

  if((other_server == NULL) && (strcmp("primary", curr_node) == 0)) {
    printf("Backup server not online\n");
    return -1;
  }

  if(msg == NULL){
    perror("uninitialized message");
    return -1;
  }

  if(msg->message_t->opcode < 10 || msg->message_t->opcode > 70 ||
      msg->message_t->c_type < 10 || msg->message_t->c_type > 60){
    perror("invalid opcode or ctype");
    return -1;
  }

  switch(msg->message_t->opcode){

    case 10: //OP_SIZE
      res = -1;
      if(msg->message_t->c_type == 60){
        msg->message_t->opcode = 10+1;
        msg->message_t->c_type = 50;
        msg->message_t->result = tree_size(tree);
        res = 0;
      }
      break;

    case 60: //OP_HEIGHT
      res = -1;
      if(msg->message_t->c_type == 60){
        msg->message_t->opcode = 60+1;
        msg->message_t->c_type = 50;
        msg->message_t->result = tree_height(tree);
        res = 0;
      }
      break;

    case 20: //OP_DEL
      res = -1;
      if(msg->message_t->c_type != 10)
        break;

      if(tree_get(tree, msg->message_t->key) == NULL){
        msg->message_t->opcode = 99;
        msg->message_t->c_type = 60;
        break;
      }
      op_n = last_assigned;

      task = malloc(sizeof(struct task_t));
      task->op_n = op_n;
      task->op = 0;
      task->key = msg->message_t->key;

      msg->message_t->opcode = 20+1;
      msg->message_t->c_type = 50;
      msg->message_t->op_n = op_n;

      last_assigned++;

      queue_add_task(task);
      res = 0;
      break;

    case 30: //OP_GET
      res = -1;
      if(msg->message_t->c_type == 10){
        struct data_t *data;
        msg->message_t->opcode = 30+1;
        if((data = tree_get(tree, msg->message_t->key)) == NULL){
          msg->message_t->c_type = 60;
          msg->message_t->data = NULL;
          msg->message_t->data_size = 0;
          res = -1;
        } else {
          msg->message_t->c_type = 20;
          msg->message_t->data = data->data;
          msg->message_t->data_size = data->datasize;
          res = 0;
        }
      }
      break;

    case 40: //OP_PUT
      res = -1;
      if(msg->message_t->c_type == 30){

        op_n = last_assigned;

        task = malloc(sizeof(struct task_t));
        task->op_n = op_n;
        task->op = 1;
        task->key = msg->message_t->key;
        task->data =
          data_create2(msg->message_t->data_size, msg->message_t->data);

        msg->message_t->opcode = 40+1;
        msg->message_t->c_type = 60;
        msg->message_t->op_n = op_n;

        last_assigned++;

        queue_add_task(task);

        res = 0;
      }
      break;

    case 50: //OP_GETKEYS
      res = -1;
      if(msg->message_t->c_type == 60){
        char **keys = tree_get_keys(tree);
        msg->message_t->opcode = 50+1;
        msg->message_t->c_type = 40;
        msg->message_t->keys = keys;
        int i = 0;

        while(strcmp(keys[i],"\0") != 0)
          i++;

        msg->message_t->n_keys = i+1;
        res = 0;
      }
      break;

    case 70: //OP_VERIFY
      if(msg->message_t->c_type == 50){
        int verified;
        if((verified = verify(msg->message_t->op_n)) < 0) {
          msg->message_t->opcode = 99;
          msg->message_t->c_type = 60;
          res = -1;
          break;
        }
        msg->message_t->opcode = 70+1;
        msg->message_t->c_type = 50;
        msg->message_t->result = verified;
        res = 0;
      }
      break;
  }

  return res;
}
