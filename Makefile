BUILTIN="bin"

all:
	g++ -I$(BUILTIN) main.cpp -o quash

clean:
	rm quash
