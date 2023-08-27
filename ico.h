/* vim:set sts=2:sw=2: */
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PNG 0x474e5089
#define MAXFILENAMELENGTH 255
#define MAXHW 0XFFFF
#define TRUE 1
#define ONE '*'
#define ZERO ' '
#define MASKBIT(dw) dw & 1 ? ZERO : ONE

// Rightshift of 32 bit colors 
#define RED 0
#define GREEN 8
#define BLUE 16

typedef struct {
  int           extract; // Extract: 1. No extract: 0.
  u_int16_t     fileno; // Number of bitmap in .ico file
  char          filename[MAXFILENAMELENGTH + 1]; // Name of file to be read
  int           show_mask; // Show icAND in ASCII art
  int           colortable; // Show colortable
} arguments;

typedef struct
{
  u_int16_t     idReserved;   // Reserved (must be 0)
  u_int16_t     idType;       // Resource Type (1 for icons)
  u_int16_t     idCount;      // How many images?
} ICONDIR;
#define IDIR_SZ sizeof(ICONDIR)

typedef struct 
{
  u_int8_t      bWidth;          // Width, in pixels, of the image
  u_int8_t      bHeight;         // Height, in pixels, of the image
  u_int8_t      bColorCount;     // Number of colors in image (0 if >=8bpp)
  u_int8_t      bReserved;       // Reserved ( must be 0)
  u_int16_t     wPlanes;         // Color Planes
  u_int16_t     wBitCount;       // Bits per pixel
  u_int32_t     dwBytesInRes;    // How many bytes in this resource?
  u_int32_t     dwImageOffset;   // Where in the file is this image?
} ICONDIRENTRY;
#define IDEN_SZ sizeof(ICONDIRENTRY)

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

void fferror(FILE* f, char* msg) {
  fprintf(stderr, "Error reading .ico file: %s\n", msg);
  fclose(f);
  exit(EXIT_FAILURE);
}

void memerror(char* msg) {
  fprintf(stderr, "Memory allocation error: %s", msg);
}

arguments parseargs(int argc, char** argv);
int zero256(u_int8_t u) { return u ? u : 256; }
void print_direntry(ICONDIRENTRY* direntry);
int read_bmphdr(FILE* f, long offset, BITMAPINFOHEADER* hdr);
int read_colortbl(FILE* f, BITMAPINFOHEADER* hdr);
int show_icon_mask(FILE* f, ICONDIRENTRY* direntry, BITMAPINFOHEADER* hdr);
int extract_bmp(FILE* f, ICONDIRENTRY* idir, arguments* args);
void helpmsg();
