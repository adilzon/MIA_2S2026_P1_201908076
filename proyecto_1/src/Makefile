ifeq ($(OS),Windows_NT)
    LIBS_API = -lws2_32
    RM_CMD = rm -f run run.exe run_api run_api.exe *.o ../include/*.o ../include/*.cpp.o 2>nul || del /f /q run.exe run_api.exe *.o ..\include\*.o 2>nul || true
else
    LIBS_API =
    RM_CMD = rm -f run run.exe run_api run_api.exe *.o ../include/*.o ../include/*.cpp.o
endif

CXX = g++
CXXFLAGS = -std=c++17 -I../lib

OBJECTS = main.o ../include/scanner.o ../include/disco.o ../include/mount.o ../include/shared.o ../include/filesystem.o ../include/filemanager.o ../include/report.o ../include/users.o
API_OBJECTS = ../include/scanner.o ../include/disco.o ../include/mount.o ../include/shared.o ../include/filesystem.o ../include/filemanager.o ../include/report.o ../include/users.o

all: run run_api

run: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

run_api: api_main.cpp $(API_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ api_main.cpp $(API_OBJECTS) -pthread $(LIBS_API)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o $@

../include/scanner.o: ../include/scanner.cpp
	$(CXX) $(CXXFLAGS) -c ../include/scanner.cpp -o $@

../include/disco.o: ../include/disco.cpp
	$(CXX) $(CXXFLAGS) -c ../include/disco.cpp -o $@

../include/mount.o: ../include/mount.cpp
	$(CXX) $(CXXFLAGS) -c ../include/mount.cpp -o $@

../include/shared.o: ../include/shared.cpp
	$(CXX) $(CXXFLAGS) -c ../include/shared.cpp -o $@

../include/filesystem.o: ../include/filesystem.cpp
	$(CXX) $(CXXFLAGS) -c ../include/filesystem.cpp -o $@

../include/filemanager.o: ../include/filemanager.cpp
	$(CXX) $(CXXFLAGS) -c ../include/filemanager.cpp -o $@

../include/report.o: ../include/report.cpp
	$(CXX) $(CXXFLAGS) -c ../include/report.cpp -o $@

../include/users.o: ../include/users.cpp
	$(CXX) $(CXXFLAGS) -c ../include/users.cpp -o $@

clean:
	-$(RM_CMD)

.PHONY: all clean run run_api