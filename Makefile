all:
	gcc -o run WLGenerator.c -lrt

clean:
	rm -rf run
