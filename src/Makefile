BUILD := ../build
CC := gcc
INC := ./headers
CFLAGS := -I $(INC) -g -Wall 
all :	$(BUILD)/netlib.o\
	$(BUILD)/list.o\
	$(BUILD)/hash_table.o\
	$(BUILD)/fd_manager.o\
	$(BUILD)/easy_epoll.o\
	$(BUILD)/packet.o\
	$(BUILD)/auto_match.o\
	$(BUILD)/proxy
	echo build finished
	
$(BUILD)/easy_epoll.o : easy_epoll.c $(INC)/easy_epoll.h
	$(CC) -c easy_epoll.c $(CFLAGS) -o $(BUILD)/easy_epoll.o
$(BUILD)/netlib.o : netlib.c $(INC)/netlib.h
	$(CC) -c netlib.c $(CFLAGS) -o $(BUILD)/netlib.o
$(BUILD)/list.o : list.c $(INC)/list.h
	$(CC) -c list.c $(CFLAGS) -o $(BUILD)/list.o
$(BUILD)/hash_table.o : hash_table.c $(INC)/hash_table.h $(INC)/list.h 
	$(CC) -c hash_table.c $(CFLAGS) -o $(BUILD)/hash_table.o
$(BUILD)/fd_manager.o : fd_manager.c $(INC)/fd_manager.h $(INC)/hash_table.h \
			$(INC)/list.h $(INC)/netlib.h
	$(CC) -c fd_manager.c $(CFLAGS) -o $(BUILD)/fd_manager.o
$(BUILD)/auto_match.o : auto_match.c $(INC)/auto_match.h 
	$(CC) -c auto_match.c $(CFLAGS) -o $(BUILD)/auto_match.o
$(BUILD)/packet.o : packet.c $(INC)/packet.h
	$(CC) -c packet.c $(CFLAGS) -o $(BUILD)/packet.o
$(BUILD)/proxy : $(BUILD)/packet.o $(BUILD)/auto_match.o $(BUILD)/fd_manager.o\
$(BUILD)/hash_table.o $(BUILD)/list.o $(BUILD)/netlib.o $(BUILD)/easy_epoll.o
	$(CC) __server.c  $(BUILD)/auto_match.o $(BUILD)/packet.o $(BUILD)/fd_manager.o $(BUILD)/hash_table.o $(BUILD)/list.o $(BUILD)/netlib.o $(BUILD)/easy_epoll.o $(CFLAGS) -o proxy
clean:
	rm $(BUILD)/*.o
