CC = gcc
CFLAGS = -g -Wall -ansi -pedantic
SRCMODULS = list.c argv_util.c process_util.c string_util.c parser.c
OBJMODULS = $(SRCMODULS:.c=.o)
EXECUTABLE = clemulator.out

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

$(EXECUTABLE): main.c $(OBJMODULS)
	$(CC) $(CFLAGS) $^ -o $@

run: $(EXECUTABLE) 
	./$(EXECUTABLE)

clean:
	rm -f *.o $(EXECUTABLE) 
