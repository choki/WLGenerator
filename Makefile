all:
	gcc -o run WLGenerator.c -lrt

clean:
	rm -rf run
	rm -rf result.txt
	rm -rf test.txt
