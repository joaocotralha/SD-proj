######### Grupo 59 #########
#   João Cotralha Nº51090  #
#  Cláudio Esteves Nº51098 #
#  José Salgueiro Nº50004  #
############################

IDIR = include
CFLAGS = -I $(IDIR)
CC = gcc
LD = ld

C_DEPS = data.o entry.o message.o client_stub.o network_client.o sdmessage.pb-c.o
S_DEPS = data.o entry.o tree.o message.o network_server.o tree_skel.o client_stub.o network_client.o sdmessage.pb-c.o
LIBS = -lrt -lpthread -lzookeeper_mt

ODIR = object

all:
	make protobuf
	make tree-client
	make tree-server

%.o: source/%.c
	$(CC) -o $(ODIR)/$@ -c $< $(CFLAGS)

client_stub.o: source/client_stub.c
	$(CC) -o $(ODIR)/$@ -c $< $(CFLAGS) -DTHREADED

tree_skel.o: source/tree_skel.c
	$(CC) -o $(ODIR)/$@ -c $< $(CFLAGS) -DTHREADED

protobuf: sdmessage.proto
	protoc --c_out=. sdmessage.proto
	touch sdmessage.pb-c.h
	touch sdmessage.pb-c.c
	mv -f sdmessage.pb-c.h include
	mv -f sdmessage.pb-c.c source

client-lib.o: $(C_DEPS)
	$(LD) -r -o lib/client-lib.o $(addprefix $(ODIR)/,$(C_DEPS))

server-lib.o: $(S_DEPS)
	$(LD) -r -o lib/server-lib.o $(addprefix $(ODIR)/,$(S_DEPS))

tree-client: tree_client.o client-lib.o
	$(CC) object/tree_client.o lib/client-lib.o -I include -L /usr/local/lib \
	-lprotobuf-c -o binary/tree-client $(LIBS)

tree-server: tree_server.o server-lib.o
	$(CC) object/tree_server.o lib/server-lib.o -I include -L /usr/local/lib \
	-lprotobuf-c -lprotobuf -o binary/tree-server $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*
	rm -f binary/*
	rm -f lib/*
	rm -f source/sdmessage.pb-c.c
	rm -f $(IDIR)/sdmessage.pb-c.h
