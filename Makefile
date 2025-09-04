CXX=g++
CXXFLAGS=-std=c++17 -Iinclude -I/usr/include -O2
LIBS=`pkg-config --libs glfw3` -lGLEW -lGL
SRCS=src/*.cpp
all:
	$(CXX) $(CXXFLAGS) -o model_genrator src/*.cpp -Iinclude $(LIBS)
clean:
	rm -f model_genrator
