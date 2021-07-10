#ifndef _TREE_SKEL_H
#define _TREE_SKEL_H

#include "zookeeper/zookeeper.h"

#include "sdmessage.pb-c.h"
#include "message-private.h"
#include "tree.h"

struct thread_parameters {
	char *a_string;
	int a_int;
};

struct task_t{
  struct task_t *next; //proxima task na fila
  int op_n; //o número da operação
  int op; //a operação a executar. op=0 se for um delete, op=1 se for um put
  char *key; //a chave a remover ou adicionar
  struct data_t *data; // os dados a adicionar em caso de put, ou NULL em caso de delete
};

/* Inicia o skeleton da árvore.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int tree_skel_init();

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy();

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(struct message_t *msg);

/* Verifica se a operação identificada por op_n foi executada.
 * devolve 0 se nao foi executada, caso contrario 1 ou -1 em erro
 */
int verify(int op_n);

int tree_zoo(char* server_port, char* zoo_port);

void server_conn_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

static void server_child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

#endif
