#include <stdio.h>
#include <compress.h>

int main(int argc, const char *argv[]){
	
	//Check for proper input
	if (argc != 2) {
		printf ("Compress by Scott Bertics (2013)\n");
		printf ("Usage: ./compress infile\n");
		return 0;
	}
	compressFile(argv[1],stdout);
	return 0;
}