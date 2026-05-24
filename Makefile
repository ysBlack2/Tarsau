CC = gcc
CFLAGS = -Wall -Wextra
TARGET = tarsau

all: $(TARGET)

$(TARGET): tarsau.c
	$(CC) $(CFLAGS) -o $(TARGET) tarsau.c

clean:
	rm -f $(TARGET)