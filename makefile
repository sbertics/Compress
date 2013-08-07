all:
	gcc -c src/compress.c -I include -std=c99 -o compress.o
	gcc app/app_compress.c compress.o -I include -std=c99 -o bin/compress
	gcc app/app_decompress.c compress.o -I include -std=c99 -o bin/decompress
	rm compress.o
