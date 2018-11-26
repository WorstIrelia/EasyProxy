BUILD := ./build
TARGET := ./bin
SRC := ./src
CC := gcc
INC := ./src/headers
CFLAGS := -I $(INC) -g -Wall -std=gnu99
all :	$(BUILD)/netlib.o\
	$(BUILD)/list.o\
	$(BUILD)/hash_table.o\
	$(BUILD)/fd_manager.o\
	$(BUILD)/easy_epoll.o\
	$(BUILD)/packet.o\
	$(BUILD)/auto_match.o\
	$(BUILD)/init.o\
	$(TARGET)/proxy
	echo build finished
	
$(BUILD)/easy_epoll.o : $(SRC)/easy_epoll.c $(INC)/easy_epoll.h
	$(CC) -c $(SRC)/easy_epoll.c $(CFLAGS) -o $(BUILD)/easy_epoll.o
$(BUILD)/netlib.o : $(SRC)/netlib.c $(INC)/netlib.h
	$(CC) -c $(SRC)/netlib.c $(CFLAGS) -o $(BUILD)/netlib.o
$(BUILD)/list.o : $(SRC)/list.c $(INC)/list.h
	$(CC) -c $(SRC)/list.c $(CFLAGS) -o $(BUILD)/list.o
$(BUILD)/hash_table.o : $(SRC)/hash_table.c $(INC)/hash_table.h $(INC)/list.h 
	$(CC) -c $(SRC)/hash_table.c $(CFLAGS) -o $(BUILD)/hash_table.o
$(BUILD)/fd_manager.o : $(SRC)/fd_manager.c $(INC)/fd_manager.h $(INC)/hash_table.h \
			$(INC)/list.h $(INC)/netlib.h
	$(CC) -c $(SRC)/fd_manager.c $(CFLAGS) -o $(BUILD)/fd_manager.o
$(BUILD)/auto_match.o : $(SRC)/auto_match.c $(INC)/auto_match.h 
	$(CC) -c $(SRC)/auto_match.c $(CFLAGS) -o $(BUILD)/auto_match.o
$(BUILD)/packet.o : $(SRC)/packet.c $(INC)/packet.h
	$(CC) -c $(SRC)/packet.c $(CFLAGS) -o $(BUILD)/packet.o
$(BUILD)/init.o : $(SRC)/init.c
	$(CC) -c $(SRC)/init.c $(CFLAGS) -o $(BUILD)/init.o
$(TARGET)/proxy : $(BUILD)/packet.o $(BUILD)/auto_match.o $(BUILD)/fd_manager.o\
$(BUILD)/hash_table.o $(BUILD)/list.o $(BUILD)/netlib.o $(BUILD)/easy_epoll.o\
$(BUILD)/init.o $(SRC)/__server.c
	$(CC) $(SRC)/__server.c  $(BUILD)/init.o $(BUILD)/auto_match.o $(BUILD)/packet.o $(BUILD)/fd_manager.o $(BUILD)/hash_table.o $(BUILD)/list.o $(BUILD)/netlib.o $(BUILD)/easy_epoll.o $(CFLAGS) -o $(TARGET)/proxy
clean:
	rm $(BUILD)/*.o
	rm $(TARGET)/*
