CC= g++
#CFLAGS = -g -O2 -fpermissive
CFLAGS = -g -fsanitize=address -static-libasan -fpermissive
OBJS = simulation.o process.o gc.o
TARGET = simul

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


simulation.o: simulation.cpp settings.h process.h
	$(CC) $(CFLAGS) -c $< -o $@

process.o: process.cpp process.h gc.h settings.h
	$(CC) $(CFLAGS) -c $< -o $@

gc.o: gc.cpp gc.h process.h settings.h
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f *.o
	rm -f $(TARGET)
