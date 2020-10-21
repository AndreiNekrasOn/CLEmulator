CC = gcc
CFLAGS = -g -Wall -Werror -ansi -pedantic
SRCMODULS = list.c argv_processing.c
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
