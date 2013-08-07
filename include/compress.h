#define EOF_CHARACTER 256
#define NO_CHARACTER 257
#define FREQ_TABLE_SZ 257

int compressFile(const char *infile, FILE *outfile);
int decompressFile(const char *infile, FILE *outfile);