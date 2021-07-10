/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "message-private.h"
#include "sdmessage.pb-c.h"

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int write_all(int sock, char *buf, int len){
	int buf_size = len;

	signal(SIGPIPE, SIG_IGN);

  while(len > 0) {
      int res = write(sock, buf, len);

      if(res < 0 && errno != EINTR)
          perror("write fail");
			if(res < 1 && errno != EINTR)
				return res;

      buf += res;
      len -= res;
  }
  return buf_size;
}

int read_all(int sock, char *buf, int len){
	int buf_size = len;

	signal(SIGPIPE, SIG_IGN);

  while(len > 0) {
      int res = read(sock, buf, len);

			if(res < 0 && errno != EINTR)
          perror("write fail");
			if(res < 1 && errno != EINTR)
				return res;

      buf += res;
      len -= res;
  }
  return buf_size;
}

void message_init(struct message_t *msg){
	msg->message_t = malloc(sizeof(struct _MessageT));
	message_t__init(msg->message_t);
	msg->bmessage_t = *msg->message_t;

}
