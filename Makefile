CC= g++
CFLAGS = -g -O2 -fpermissive
#CFLAGS = -g -fsanitize=address -static-libasan -fpermissive
OBJS = main.o map.o gc.o
TARGET = simul

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


simulation.o: main.cpp settings.h map.h
	$(CC) $(CFLAGS) -c $< -o $@

process.o: map.cpp map.h gc.h settings.h
	$(CC) $(CFLAGS) -c $< -o $@

gc.o: gc.cpp gc.h map.h settings.h
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f *.o
	rm -f $(TARGET)
