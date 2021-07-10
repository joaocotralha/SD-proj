/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */

#define NFDESC 10

#include "tree_skel.h"
#include "sdmessage.pb-c.h"
#include "message-private.h"
#include "network_server.h"


#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>

int sockfd;

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port){
  struct sockaddr_in socket_in;

  if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
    perror("server socket creation fail");
    return -1;
  }

  int enable = 1;
  if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
    perror("socket option set fail");
    close(sockfd);
    return -1;
  }

  socket_in.sin_family = AF_INET;
  socket_in.sin_port = htons(port);
  socket_in.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(sockfd, (struct sockaddr *) &socket_in, sizeof(socket_in)) < 0){
    perror("socket bind fail");
    close(sockfd);
    return -1;
  }

  if(listen(sockfd, 0) < 0){
    perror("socket listen fail");
    close(sockfd);
    return -1;
  }

  return sockfd;
}

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket){
  struct sockaddr_in client;
  struct pollfd connections[NFDESC];
  struct message_t *msg;
  int connsockfd;
  int size_client;
  int kfds;

  for(int i = 0; i < NFDESC; i++) {
    connections[i].fd = -1;
    connections[i].events = 0;
    connections[i].revents = 0;
  }
  connections[0].fd = listening_socket;
  connections[0].events = POLLIN;

  int nfds = 1;

  size_client = sizeof(struct sockaddr);

  printf("Servidor à espera de ligações:\n");
  while((kfds = poll(connections, nfds, 10)) >= 0){
    if (kfds > 0) {

      if((connections[0].revents & POLLIN) && (nfds < NFDESC)){
        if ((connections[nfds].fd = accept(connections[0].fd,
              (struct sockaddr *) &client, &size_client)) > 0){
          connections[nfds].events = POLLIN;
          nfds++;
        }
      }
      for(int i = 1; i < nfds; i++){ // Iterar todas as ligacoes

        if(connections[i].revents & POLLIN) {

          printf("  A receber mensagem do cliente %d...\n",i);
          if((msg = network_receive(connections[i].fd)) == NULL){
            printf("  Ligação fechada pelo cliente.\n");
            close(connections[i].fd);
            connections[i].fd = -1;
            nfds--;
            printf("Servidor à espera de ligações:\n");
            continue;
          }

          if(invoke(msg) < 0)
            printf("  Failure invoking msg\n");

          printf("  A enviar resposta ao cliente %d...\n",i);
          if(network_send(connections[i].fd, msg) < 0){
            printf("  Ligação fechada pelo cliente.\n");
            close(connections[i].fd);
            connections[i].fd = -1;
            nfds--;
          }
          free(msg->message_t);
          free(msg);

          printf("Servidor à espera de ligações:\n");
        }
        if((connections[i].revents & POLLERR) ||
          (connections[i].revents & POLLHUP)){
          close(connections[i].fd);
          connections[i].fd = -1;
          nfds--;
        }
      }
    }
  }
  close(listening_socket);
  return 0;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
struct message_t *network_receive(int client_socket){
  struct message_t *msg = malloc(sizeof(struct message_t));
  int res, len;

  message_init(msg);

  if(client_socket < 0){
    return NULL;
  }

  len = 0;
  if((res = read_all(client_socket, (char *) &len, sizeof(int))) < 0)
    perror("server msg length receive failure");
  if(res < 1) {
    free(msg->message_t);
    free(msg);
    return NULL;
  }


  int len_recv = ntohl(len);
  void *buf_recv = malloc(len_recv);

  if((res = read_all(client_socket, buf_recv, len_recv)) < 0)
    perror("server msg receive failure");
  if(res < 1){
    free(msg->message_t);
    free(msg);
    free(buf_recv);
    return NULL;
  }

  free(msg->message_t);
  if((msg->message_t = message_t__unpack(NULL, len_recv, buf_recv)) == NULL){
    perror("server receive unpack failure");
    free(msg->message_t);
    free(msg);
    free(buf_recv);
    return NULL;
  }

  free(buf_recv);
  return msg;
}


/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, struct message_t *msg){
  void *buf_send;
  int res,len,len_send;

  if(msg == NULL)
    return -1;

  msg->bmessage_t = *msg->message_t;
  len = message_t__get_packed_size(&msg->bmessage_t);

  if((buf_send = malloc(len)) == NULL){
    return -1;
  }

  message_t__pack(&msg->bmessage_t, buf_send);

  len_send = htonl(len);

  if((res = write_all(client_socket, (char *)&len_send, sizeof(int))) < 0)
    perror("server msg length send failure");
  if(res < 1) {
    free(buf_send);
    return -1;
  }

  if((res = write_all(client_socket, buf_send, len)) < 0)
    perror("server msg send failure");
  if(res < 1){
    free(buf_send);
    return -1;
  }

  free(buf_send);
  return 0;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close(){
  close(sockfd);
}
