TARGET = test_vector
CC     = gcc
CFLAGS = -Wall -Wextra -g -pedantic
OBJS = vector.o test_vector.o

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

vector.o: vector.c vector.h
	$(CC) $(CFLAGS) -c vector.c

test_vector.o: test_vector.c vector.h
	$(CC) $(CFLAGS) -c test_vector.c

clean:
	rm -f $(TARGET) $(OBJS)