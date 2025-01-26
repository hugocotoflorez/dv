CC = cc -Wall -Wextra -O0 -fsanitize=address,null
CC2 = cc -w
INC = -I./include
LIB = $(wildcard ./*.h)
OUT = dv
SRC = $(wildcard ./*.c)
OBJ_DIR = ./objs
OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))

$(OUT): $(OBJ)
	mkdir -p $(OBJ_DIR)
	$(CC) $(OBJ) $(INC) -o $(OUT)

$(OBJ_DIR)/%.o: %.c $(LIB)
	mkdir -p $(dir $@)
	$(CC) -c $< $(INC) -o $@

clean:
	rm -rf $(OBJ_DIR)

install: $(OUT)
	mv $(OUT) ~/.local/bin/$(OUT)
	chmod +x ~/.local/bin/$(OUT)
