CXX=g++
CXXFLAGS=-O2 -Wall -W -pedantic

all: add2groups.o extend_ds.o
	$(CXX) $(CXXFLAGS) -o add2groups add2groups.o -lhdf5_cpp -lhdf5
	$(CXX) $(CXXFLAGS) -o extend_ds extend_ds.o -lhdf5_cpp -lhdf5
