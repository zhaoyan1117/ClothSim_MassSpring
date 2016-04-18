CC = g++-4.9

ifeq ($(shell sw_vers 2>/dev/null | grep Mac | awk '{ print $$2}'),Mac)
	CFLAGS = -w -g -O3 -DGL_GLEXT_PROTOTYPES -I./include/ -I/usr/X11/include -DOSX
	LDFLAGS = -framework GLUT -framework OpenGL \
		-L"/System/Library/Frameworks/OpenGL.framework/Libraries" \
		-L./library \
		-lGL -lGLU -lm -lstdc++ -lGLEW -lfreeimage
else
	CFLAGS = -I./include/
	LDFLAGS = -lGL -lGLU -lglut -lGLEW -lfreeimage
endif

RM = /bin/rm -f
all: main
main: src/clothSim.o
	$(CC) $(CFLAGS) -o clothSim src/clothSim.o $(LDFLAGS)
src/clothSim.o: src/clothSim.cpp src/ClothSim.h
	$(CC) $(CFLAGS) -c src/clothSim.cpp -o src/clothSim.o
clean:
	$(RM) *.o src/*.o clothSim
