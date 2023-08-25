/* vim:set sts=2:sw=2: */
// A short programm to analyze the header of a .bmp file
#define _XOPEN_SOURCE
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXFILENAMELENGTH 256

// Rightshift of 32 bit colors 
#define xcolor(C, Shift) (u_int8_t)((C >> Shift) & 0xFF)
#define RED 2
#define GREEN 4
#define BLUE 6

typedef struct {
  char          filename[MAXFILENAMELENGTH + 1]; // Name of file to be read
} arguments;

typedef struct {
  u_int32_t     size;
  u_int16_t     reserved1;
  u_int16_t     reserved2;
  u_int32_t     offset;
} BITMAPFILEHEADER;
#define BMPFH_SZ sizeof(BITMAPFILEHEADER)

typedef struct {
  u_int32_t     biSize;
  int           biWidth;
  int           biHeight;
  u_int16_t     biPlanes;
  u_int16_t     biBitCount;
  u_int32_t     biCompression;
  u_int32_t     biSizeImage;
  int           biXPelsPerMeter;
  int           biYPelsPerMeter;
  u_int32_t     biClrUsed;
  u_int32_t     biClrImportant;
} BITMAPINFOHEADER;
#define BMIH_SZ sizeof(BITMAPINFOHEADER)

void fferror(FILE* f, char* filename) {
  fprintf(stderr, "Not a valid .bmp file: %s\n", filename);
  fclose(f);
  exit(1);
}

void memerror(char* msg) {
  fprintf(stderr, "Memory allocation error: %s", msg);
}

arguments parseargs(int argc, char** argv);
int zero256(u_int8_t u) { return u ? u : 256; }
void read_bmphdr(FILE* f, char* filename); 
void read_colortbl(FILE* f, BITMAPINFOHEADER* hdr);

int main(int argc, char** argv) {

  FILE* f;
  arguments args;
  char magic[2];
  BITMAPFILEHEADER bmp_fhdr;
  size_t bytes_read; // Number of bytes read from file

  args = parseargs(argc, argv);

  if ((f = fopen(args.filename, "r")) == NULL) {
    error(1, errno, "Can't open file %s\n", args.filename);
  }

  bytes_read = fread(&magic, 1, 2, f);
  if ((bytes_read != 2) || strcmp(magic, "BM")) {
    fferror(f, args.filename);
  }

  bytes_read = fread(&bmp_fhdr, 1, BMPFH_SZ, f);
  if (bytes_read != BMPFH_SZ)  {
    fferror(f, args.filename);
  }

  printf("Filesize: %u bytes\n", bmp_fhdr.size);
  read_bmphdr(f, args.filename);

  fclose(f);

  return 0;
}

void read_bmphdr(FILE* f, char* filename) {

  BITMAPINFOHEADER hdr; 

  if (fseek(f, 14, SEEK_SET)) {
    error(1, errno, "In read_bmphdr");
  }
  
  if (fread(&hdr, 1, BMIH_SZ, f) != BMIH_SZ) {
      fferror(f, filename);
  }

  puts("Bitmap Info:");
  printf("biSize          %u\n", hdr.biSize);
  printf("biWidth         %d\n", hdr.biWidth);
  printf("biHeight        %d\n", hdr.biHeight);
  printf("biPlanes        %hu\n", hdr.biPlanes);
  printf("biBitCount      %hu\n", hdr.biBitCount);
  printf("biCompression   %u\n", hdr.biCompression);
  printf("biSizeImage     %u\n", hdr.biSizeImage);
  printf("biXPelsPerMeter %d\n", hdr.biXPelsPerMeter);
  printf("biYPelsPerMeter %d\n", hdr.biYPelsPerMeter);
  printf("biClrUsed       %u\n", hdr.biClrUsed);
  printf("biClrImportant  %u\n", hdr.biClrImportant);

  if (hdr.biBitCount <= 8) { // There is a color table
    read_colortbl(f, &hdr);
  }
}

void read_colortbl(FILE* f, BITMAPINFOHEADER* hdr) {
  /* Try to identify color table (palette) entries between the end of the 
   * BITMAPINFOHEADER and the beginning of the Image. The beginning of the 
   * colortbl est à direntry->dwImageOffset + BMIH_SZ.
   * The end is at direntry->dwImageOffset + direntry->dwBytesInRes -
   * hdr->biSizeImage.
   */
  u_int32_t i, color, num_colors;

  num_colors = hdr->biClrUsed ? hdr->biClrUsed : 1 << hdr->biBitCount;

  puts("Color table:");
  for (i=0; i<num_colors; i++) {
    if (fread(&color, 1, 4, f) < 4)
      break;
    printf("Color %3u: %2X %2X %2X\n", i, xcolor(color, RED), 
        xcolor(color, GREEN), xcolor(color, BLUE));
  }
}

arguments parseargs(int argc, char** argv) {

  arguments args; 

  if (argc < 2) {
    fprintf(stderr, "No argument provided. Filename is mandatory.\n");
    exit(1);
  }

  memset(&args, 0, sizeof(arguments));

  strcpy(args.filename, argv[1]);

  return args;
}

