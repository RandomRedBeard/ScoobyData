CC = gcc
CFLAG = -Wall -c
LFLAG = -lcurses

TARGET = ScoobyLogger

OBJS = scooby_logger.o

%.o: %.c
	$(CC) $(CFLAG) -o $@ $<

all: $(TARGET)

clean:
	rm $(OBJS)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LFLAG) -o $(TARGET)
