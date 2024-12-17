# Makefile for the smash program
CC = g++
CFLAGS = -g -Wall -std=c++11
CCLINK = $(CC)
OBJS = smash.o commands.o
RM = rm -f
TARGET = smash

all: $(TARGET)
# Creating the  executable
$(TARGET): $(OBJS)
	$(CCLINK) -o $(TARGET) $(OBJS)
# Creating the object files
commands.o: commands.cpp commands.h
	$(CC) $(CFLAGS) -c commands.cpp
smash.o: smash.cpp commands.h signals.h
	$(CC) $(CFLAGS) -c smash.cpp
# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ core.*

