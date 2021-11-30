CC = g++
 
CFLAGS  = -g -Wall -Werror

TARGET = client

all: $(TARGET)

$(TARGET): $(TARGET).cc
		$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cc

clean:
		$(RM) $(TARGET)