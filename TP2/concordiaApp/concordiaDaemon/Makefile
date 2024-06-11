CC=gcc
CFLAGS=-I./includes -I../concordiaCommon/includes
DEPS = ../concordiaCommon/includes/mensagem.h
OBJ_DIR = build
SRC_DIR = src
CMN_DIR = ../concordiaCommon/src
OBJCONC = $(OBJ_DIR)/concordia-app.o $(OBJ_DIR)/mensagem.o


# Rule to create build directory if it doesn't exist
$(shell mkdir -p $(OBJ_DIR))

# Rule to compile .c files into .o files
$(OBJ_DIR)/%.o: $(CMN_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Rule to build executables
concordia-app: $(OBJCONC)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(OBJ_DIR) concordia-app


.DEFAULT_GOAL = concordia-app