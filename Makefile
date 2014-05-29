CXX=g++
CXXFLAGS=-O2 -Wall -W -pedantic

all: add2groups.o 
	$(CXX) $(CXXFLAGS) -o add2groups add2groups.o -lhdf5_cpp -lhdf5
