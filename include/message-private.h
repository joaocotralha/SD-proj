/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

#include "client_stub.h"
#include "sdmessage.pb-c.h"
#include "zookeeper/zookeeper.h"

#include <netinet/in.h>

struct message_t {
  struct  _MessageT *message_t; // buff to msg
  struct  _MessageT bmessage_t; //msg to buff
};

struct rtree_t{
  struct sockaddr_in socket;
  int server_flag;
  int descritor;
};

void message_init(struct message_t *msg);

int write_all(int sock, char *buf, int len);

int read_all(int sock, char *buf, int len);

#endif
