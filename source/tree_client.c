/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "netinet/in.h"
#include "client_stub.h"
#include "message-private.h"
#include "data.h"
#include "entry.h"
#include "network_client.h"
#include "tree.h"
#include "tree-private.h"

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////

int main(int argc, char const *argv[]) {

  struct rtree_t *rtree;
  struct data_t *data;
  struct entry_t *entry;
  int length, size, height, res;
  char input[81], str[80];
  char *com[3];
  char *token;
  char **keys;


  if (argc != 3){

      printf("Use: ./tree-client <server_ip> <server_port>\n");
      printf("Example: ./tree-client 127.0.0.1 12345\n");
      return -1;
  }

  // "<server ip>:<server port>"
  char *addr;
  char *ip = strdup(argv[1]);
  char *port = strdup(argv[2]);
  int str_size = strlen(ip) + strlen(port);
  if((addr = malloc(sizeof(char) * (str_size + 2))) != NULL){
      addr[0] = '\0';   // ensures the memory is an empty string
      strcat(addr,ip);
      strcat(addr, ":");
      strcat(addr,port);
      addr[str_size+1] = '\0';
  } else {
      printf("<server ip>:<server port> allocation error\n");
      return -1;
  }

  bool quit;
  quit = false;

  rtree = zoo_connect(addr);
  free(ip);
  free(port);
  free(addr);

  if (rtree == NULL) {
    printf("Could not connect.\n");
    free(rtree);
    return -1;
  }

  length = 0;

  // Cycles for more commands until client uses command quit
  while (!quit) {

    printf("A aguardar comando...\n");
    if (fgets(input, sizeof(input), stdin) == NULL) {
      perror("fgets is NULL");
      quit = true;
    }

    int i = 0;

    token = strtok(input, " ");
    while(token != NULL && (i < 3)) {
      com[i] = strdup(token);
      token = strtok(NULL, " ");
      i++;
    }

    com[i-1][strlen(com[i-1])-1] = '\0';
    length = i;
    ///////////////////////////////////////////////////////////

    int opcode = 0;

    if (strcmp(com[0],"quit") == 0)  // Command for client to quit
      quit = true;
    else if (strcmp(com[0],"size") == 0)
      opcode = 10;
    else if (strcmp(com[0],"del") == 0)
      opcode = 20;
    else if (strcmp(com[0],"get") == 0)
      opcode = 30;
    else if (strcmp(com[0],"put") == 0)
      opcode = 40;
    else if (strcmp(com[0],"getkeys") == 0)
      opcode = 50;
    else if (strcmp(com[0],"height") == 0)
      opcode = 60;
    else if (strcmp(com[0],"verify") == 0)
      opcode = 70;
    else
      opcode = 99;


    switch (opcode) {

      case 40: // Code for OP_PUT

        if (length < 3) {
          printf("Parametros invalidos\nExemplo PUT: put <key> <data>\n");
          if(length > 1) {
            free(com[1]);
          }
          break;
        }

        data = data_create2(strlen(com[2]), com[2]);
        entry = entry_create(com[1], data);
        if ((res = rtree_put(rtree, entry)) == -1) {
          printf("Did not put on tree.\n");
        } else {
          printf("OK. Operation ID: %d\n", res);
        }

        entry_destroy(entry);
        break;

      ///////////////////////////////////////////////////////////

      case 30 : // Code for OP_GET

        if (length < 2) {
          printf("Parametros invalidos\nExemplo GET: get <key>\n");
          break;
        }
        if ((data = rtree_get(rtree, com[1])) == NULL) {
          printf("Could not get.\n");
          free(com[1]);
          break;
        }
        char *ptr;
        ptr = data->data;
        printf("data: ");
        for (int i=0; i < data->datasize ; i++)
          printf("%c",*ptr++);
        printf("\n");

        data_destroy(data);
        free(com[1]);
        break;

      ///////////////////////////////////////////////////////////

      case 20: // Code for OP_DEL

        if (length < 2) {
          printf("Parametros invalidos\nExemplo DEL: del <key>\n");
          break;
        }

        if ((res = rtree_del(rtree, com[1])) == -1)
          printf("Key not found. \n");
        else
          printf("Key deleted. Operation ID: %d\n", res);

        free(com[1]);
        break;

      ///////////////////////////////////////////////////////////

      case 10: // Code for OP_SIZE

        if ((size = rtree_size(rtree)) == -1)
          printf("size gave -1. Error.\n");
        else
          printf("Tree size: %i\n", size);

        break;

      ///////////////////////////////////////////////////////////

      case 60:  // Code for OP_HEIGHT

        if ((height = rtree_height(rtree)) == -1)
          printf("Height gave -1. Error.\n");
        else
          printf("Tree height: %i\n", height);

        break;

      ///////////////////////////////////////////////////////////

      case 50: // Code for OP_GETKEYS

        if ((keys = rtree_get_keys(rtree)) == NULL) {
          printf("Could not get keys.\n");
          break;
        }

        printf("Keys: ");

        int i = 0;
        while(strcmp(keys[i+1],"\0") != 0) {
          printf("%s, ",keys[i]);
          i++;
        }

        printf("%s\n",keys[i]);
        rtree_free_keys(keys);

        break;

      ///////////////////////////////////////////////////////////

      case 70: //OP_VERIFY

        if (length < 2) {
          printf("Parametros invalidos\nExemplo VERIFY: verify <op_n>\n");
          break;
        }

        int op_n = atoi(com[1]);
        if ((res = rtree_verify(rtree, op_n) ) == -1) {
          printf("Could not verify.\n");
          break;
        }
        if (res)
          printf("Operation ID: %s was executed.\n", com[1]);
        else
          printf("Operation ID: %s was not executed.\n", com[1]);

        free(com[1]);
        break;

      ///////////////////////////////////////////////////////////
      default :
        if(opcode == 99) // Error code
          printf("%s: command not found\n", com[0]);
    }

    free(com[0]);
  }
  if (rtree_disconnect(rtree) == -1)
    printf("Unable to disconnect.\n");

	printf("Good bye.\n");
	return 0;
}
