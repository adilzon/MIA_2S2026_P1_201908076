CXX = g++
CXXFLAGS = -I../lib -std=c++17

OBJECTS_CONSOLA = main.o ../include/scanner.o ../include/validador.o
OBJECTS_SERVIDOR = main_server.o ../include/scanner.o ../include/validador.o ../include/api.o

all: ejecutable servidor

ejecutable: $(OBJECTS_CONSOLA)
	$(CXX) -o $@ $(OBJECTS_CONSOLA)

servidor: $(OBJECTS_SERVIDOR)
	$(CXX) -o $@ $(OBJECTS_SERVIDOR) -lpthread

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

main_server.o: main_server.cpp
	$(CXX) $(CXXFLAGS) -c main_server.cpp

../include/scanner.o: ../include/scanner.cpp
	$(CXX) $(CXXFLAGS) -c ../include/scanner.cpp -o ../include/scanner.o

../include/validador.o: ../include/validador.cpp
	$(CXX) $(CXXFLAGS) -c ../include/validador.cpp -o ../include/validador.o

../include/api.o: ../include/api.cpp
	$(CXX) $(CXXFLAGS) -c ../include/api.cpp -o ../include/api.o

clean:
	rm -f ejecutable servidor main.o main_server.o ../include/*.o