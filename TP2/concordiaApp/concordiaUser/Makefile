CC=gcc
CFLAGS=-I./includes -I../concordiaCommon/includes -lacl
DEPS = ../concordiaCommon/includes/mensagem.h
OBJ_DIR = build
SRC_DIR = src
CMN_DIR = ../concordiaCommon/src
OBJBOOT = $(OBJ_DIR)/concordia-boot.o $(OBJ_DIR)/mensagem.o
OBJENV  = $(OBJ_DIR)/concordia-enviar.o $(OBJ_DIR)/mensagem.o
OBJMAIL = $(OBJ_DIR)/concordia-mail.o $(OBJ_DIR)/mensagem.o
OBJROLE = $(OBJ_DIR)/concordia-roles.o $(OBJ_DIR)/mensagem.o
OBJMAN = $(OBJ_DIR)/concordia-manager.o $(OBJ_DIR)/mensagem.o
OBJSEN = $(OBJ_DIR)/concordia-sender.o $(OBJ_DIR)/mensagem.o
OBJREC = $(OBJ_DIR)/concordia-receiver.o $(OBJ_DIR)/mensagem.o

# Rule to create build directory if it doesn't exist
$(shell mkdir -p $(OBJ_DIR))

# Rule to compile .c files into .o files
$(OBJ_DIR)/%.o: $(CMN_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Rule to build executables
concordia-boot: $(OBJBOOT)
	$(CC) -o $@ $^ $(CFLAGS)

concordia-enviar: $(OBJENV)
	$(CC) -o $@ $^ $(CFLAGS)

concordia-mail: $(OBJMAIL)
	$(CC) -o $@ $^ $(CFLAGS)

concordia-roles: $(OBJROLE)
	$(CC) -o $@ $^ $(CFLAGS)

daemon-manager: $(OBJMAN)
	$(CC) -o $@ $^ $(CFLAGS)

daemon-sender: $(OBJSEN)
	$(CC) -o $@ $^ $(CFLAGS)

daemon-receiver: $(OBJREC)
	$(CC) -o $@ $^ $(CFLAGS)

# Clean target to delete the build directory
clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(OBJ_DIR) concordia-boot concordia-enviar concordia-mail concordia-roles daemon-manager daemon-sender daemon-receiver

all: concordia-boot concordia-enviar concordia-mail concordia-roles daemon-manager daemon-sender daemon-receiver

.DEFAULT_GOAL = all
