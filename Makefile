SRC = .
OBJ = obj
CFLAGS = -g -Wall

CTRMML_OBJS = \
			  $(OBJ)/ctrmml.o \
			  $(OBJ)/mml.o \
			  $(OBJ)/track.o \
			  $(OBJ)/song.o

CTRMML_HEADERS = \
				 $(SRC)/ctrmml.h

ctrmml: $(CTRMML_OBJS) $(CTRMML_HEADERS)
	$(CC) $(LDFLAGS) $(CTRMML_OBJS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ)
