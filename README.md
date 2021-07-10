
# Projeto de Sistemas Distribuídos 2020/2021

Concretização de um serviço de armazenação de pares chave-valor. Iremos utilizar uma árvore binária de pesquisa para armazenar informação, dada a sua elevada eficiência ao nível da pesquisa. Sendo este tipo de serviço utilizado em sistemas distribuídos torna-se imperativa a troca de informação via rede.  Neste sentido foram desenvolvidos mecanismos de serialização/deserialização a utilizar no envio/receção de informação, utilizando o **protocol buffer**. Desta forma, pretende-se a concretização da árvore binária de pesquisa num servidor, e à implementação de um cliente com uma interface de gestão do conteúdo da árvore binária. Isto implica que:

1. Através  do cliente,  o  utilizador  irá  invocar  operações,  que  serão  transformadas  em mensagens e enviadas pela rede até ao servidor.

2. Este, por sua vez interpretará essas mensagens, e:
a. Realizará as operações correspondentes na árvore local concretizada por ele.
b. Enviará posteriormente a resposta transformada em mensagem, ao cliente

3. Por sua vez, o cliente interpretará a mensagem de resposta
4. O Cliente procederá  de  acordo  com  o  resultado,  ficando  de  seguida  pronto  para  a  próxima operação.

O  objetivo será fornecer às aplicações que usariam esta árvore um  modelo de comunicação tipo **RPC**, onde vários clientes acedem a uma mesma árvore binária de pesquisa partilhada. Para tal, criámos um sistema concorrente que aceita pedidos de múltiplos clientes em simultâneo através do  uso  de multiplexagem  de I/O e que separa o tratamento de I/O do processamento de dados através do uso de *Threads*.

Mais concretamente temos, para além da multiplexagem de I/O, foram implementadas:

- Respostas assíncronas aos pedidos de escrita dos clientes, ou seja, em vez de executar imediatamente pedidos de escrita, o servidor devolve aos clientes um identificador da operação e passa a guardar os pedidos numa fila temporária para serem executados por uma thread
- Garantia de sincronização e threads no acesso à árvore e à Fila de Tarefas através do uso de Locks e Variáveis Condicionais.

No âmbito de suporte a tolerância a falhas, desenvolvemos um sistema de replicação com base no modelo de replicação passiva com primário fixo, através do serviço de coordenação **ZooKeeper**.

Uma descrição mais detalhada do projeto em conjunto com a sua especificação encontra-se dividida pelos quatro enunciados.

---

## Estrutura

`/include/`- contém os header *.h* requiridos pelo C
`/source/` - contém o código fonte do projeto
`/object/` - diretório que irá conter *object files*
`/binary/` - executáveis resultantes do *linking* de *object files*
`/lib/` - bibliotecas criadas na compilação

Encontram-se também, na root do projeto:

- README.md
- makefile: define as regras de compilação e ligação
- sdmessage.proto: esquema do *Protocol Buffer*

---

## Compilação e Execução

Em baixo encontram-se comandos principais e alguns exemplos de utilização

- **Compilação e ligação:** `make`

- **Execução de um servidor:** `./binary/tree-server <porto_servidor> <IP:Porto Zookeeper>`
Exemplo de utilização: `./binary/tree-server 12345 127.0.0.1:2181`

- **Execução de um cliente:** `./binary/tree-client <IP Zookeeper> <Porto Zookeeper>`
Exemple de utilização:  `./binary/tree-client 127.0.0.1 2181`

- **Apagar ficheiros temporários:** `make clean`

#### Importante

Tanto o executável cliente como o executável servidor não devem ser chamados com o valor no campo IP de *"localhost"*, devendo em vez disso ser utilizado o endereço IP em notação XXX.XXX.XXX.XXX.

---

## Autores

Projeto realizado por Grupo 59:

  - **João Cotralha** Nº51090
  - **Cláudio Esteves** Nº51098
            
