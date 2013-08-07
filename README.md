Compress
========

A Generic File Compression Library and Application<br>
Scott Bertics <sbertics@stanford.edu>

Compress is a library that implements the Huffman Encoding
file compression algorithm.

To use in your own application:

* #include "compress.h"
* Call the function:
* compressFile(const char *infile, FILE *outfile) or
* decompressFile(const char *infile, FILE *outfile)
* Copy the makefile provided with the default application when compiling your own
