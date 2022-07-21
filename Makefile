all: main.cpp surface.cpp
	g++ -o render main.cpp surface.cpp -O2 -lm -lGL -lGLU -lglut

run:
	./render

clean:
	rm render