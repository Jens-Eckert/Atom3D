build: ./src/main.cpp
	mkdir build; cd build; cmake ..; make;
	make shaders

main:
	cd build; ./main

shaders:
	glslc ./src/shaders/shader.vert -o ./src/shaders/vert.spv;
	glslc ./src/shaders/shader.frag -o ./src/shaders/frag.spv;

clean:
	rm -rf ./build
	rm -rf ./src/shaders/*.spv