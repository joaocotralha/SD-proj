/*
 * ######### Grupo 59 #########
 * #   João Cotralha Nº51090  #
 * #  Cláudio Esteves Nº51098 #
 * #  José Salgueiro Nº50004  #
 * ############################
 */
#include "tree_skel.h"
#include "network_server.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 1024

int checkInput(int argc, char **argv){

  if (argc != 3){
      printf("Use: ./tree-server <server_ip> <server_port>\n");
      printf("Example: ./tree-server 127.0.0.1 12345\n");
      return -1;
  } else if(atoi(argv[1]) <= 0){
    printf("Porto nao valido");
    return -1;
  }

  return 0;
}

int main(int argc, char **argv){
  int listening_socket, result;

  if(checkInput(argc, argv) < 0){
    return -1;
  }

  if((listening_socket = network_server_init(atoi(argv[1]))) < 0){
    perror("Failure creating listening socket");
    network_server_close();
    return -1;
  }

  if(tree_skel_init() < 0){
    perror("Failure initializing tree skeleton");
    network_server_close();
    return -1;
  }


  if(tree_zoo(argv[1], argv[2]) < 0){
    perror("Zookeeper connection could not be established");
    network_server_close();
    tree_skel_destroy();
    return -1;
  }
  result = network_main_loop(listening_socket);

  tree_skel_destroy();
  network_server_close();

  return result;
}
