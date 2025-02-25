build:
	mkdir build; cd build; cmake ..; make;

main:
	cd build; ./main

clean:
	rm -rf ./build