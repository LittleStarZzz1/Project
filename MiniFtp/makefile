CC = gcc
CFLAGS = -g
OBJS = sysutil.o session.o ftp_proto.o priv_parent.o str.o privsock.o tunable.o parseconf.o MiniFtp.o
LIBS = -lcrypt 
BIN = MiniFtp

$(BIN):$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -rf *.o $(BIN)
