/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "client_stub.h"
#include "sdmessage.pb-c.h"
#include "message-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

struct rtree_t;

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtree;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtree;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtree_t *rtree){
  if(rtree == NULL) {
    return -1;
  }

  if(rtree->descritor < 0) {
    perror("socket creation fail");
    return -1;
  }

  if(connect(rtree->descritor, (struct sockaddr *)&rtree->socket, sizeof(rtree->socket)) < 0) {
    perror("failure connecting socket");
    close(rtree->descritor);
    return -1;
  }

  return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtree_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
struct message_t *network_send_receive(struct rtree_t * rtree,
                                        struct message_t *msg) {
  if(rtree == NULL || msg == NULL) {
    printf("fds2");
    return NULL;
  }

  int len = message_t__get_packed_size(&msg->bmessage_t);
  char *buf_send = malloc(len);
  if(buf_send == NULL){
    printf("fd1");
    return NULL;
  }

  message_t__pack(&msg->bmessage_t, buf_send);

  int len_send = htonl(len);
  printf("  A enviar resposta...\n");
  //enviar tamanho da mensagem
  if(write_all(rtree->descritor,(char*) &len_send, sizeof(int)) < 0) {
    close(rtree->descritor);
    if(network_connect(rtree) < 0) {
      free(buf_send);
      printf("fds");
      return NULL;
    }
  }

  //enviar mensagem
  if(write_all(rtree->descritor, buf_send, len) < 0) {
    close(rtree->descritor);
    if(network_connect(rtree) < 0) {
      free(buf_send);
      return NULL;
    }
  }

  printf("  A receber resposta...\n");
  //recebe tamanho
  if(read_all(rtree->descritor, (char *) &len, sizeof(int)) < 0) {
    close(rtree->descritor);
    if(network_connect(rtree) < 0) {
      perror("reconnect fail");
      free(buf_send);
      return NULL;
    }
  }

  int len_recv = ntohl(len);
  void *buf_recv = malloc(len_recv);
  //recebe mensagem
  if(read_all(rtree->descritor, buf_recv, len_recv) < 0) {
    close(rtree->descritor);
    if(network_connect(rtree) < 0) {
      perror("reconnect fail");
      free(buf_send);
      free(buf_recv);
      return NULL;
    }
  }

  struct message_t *reply = malloc(sizeof(struct message_t));
  if((reply->message_t = message_t__unpack(NULL, len_recv, buf_recv)) == NULL){
    perror("send receive unpack failure");
    free(buf_recv);
    return NULL;
  }

  free(buf_send);
  free(buf_recv);
  return reply;
}

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtree_t * rtree){
  if(rtree == NULL || &rtree->socket == NULL) {
    return -1;
  }

  if(close(rtree->descritor) < 0) {
    return -1;
  }

  return 0;
}
