/* vim:set sts=2:sw=2: */
// A short programm to analyze the header of an .ico file
#define _XOPEN_SOURCE
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PNG 0x474e5089
#define MAXFILENAMELENGTH 255

typedef struct {
  u_int16_t     fileno; // Number of bitmap in .ico file
  char          filename[MAXFILENAMELENGTH + 1]; // Name of file to be read
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
  fprintf(stderr, "Not a valid .ico file: %s\n", filename);
  fclose(f);
  exit(1);
}

arguments parseargs(int argc, char** argv);
int zero256(u_int8_t u) { return u ? u : 256; }
void read_bmphdr(FILE* f, long offset, char* filename); 
void read_colortbl(FILE* f, ICONDIRENTRY* direntry, BITMAPINFOHEADER* hdr,
        char* filename);

int main(int argc, char** argv) {

  FILE* f;
  arguments args;
  ICONDIR iconhdr;
  ICONDIRENTRY* ic_dir;
  size_t dirbytes; // Length of ic_dir in bytes

  /* args = parseargs(argc, argv); */

  if (argc != 2) {
    fprintf(stderr, "No filename provided\n");
    exit(1);
  }

  if ((f = fopen(argv[1], "r")) == NULL) {
    error(1, errno, "Can't open file %s\n", argv[1]);
  }


  if (fread(&iconhdr, 1, IDIR_SZ, f) != IDIR_SZ) {
    fferror(f, argv[1]);
  }

  if (iconhdr.idType != 1) {
    fferror(f, argv[1]);
  }

  printf("Number of images: %u\n", iconhdr.idCount);

  dirbytes = iconhdr.idCount * IDEN_SZ;
  ic_dir = (ICONDIRENTRY*)malloc(dirbytes);


  if (fread(ic_dir, 1, dirbytes, f) != dirbytes) {
    fferror(f, argv[1]);
  }

  // Print directory entries:
  for (u_int16_t i=0; i<iconhdr.idCount; i++) {
    printf("\nImage %d\n", i);
    printf("Width x Height: %d x %d\n", 
      zero256(ic_dir[i].bWidth), zero256(ic_dir[i].bWidth));
    printf("Color Count   : %hhu\n", ic_dir[i].bColorCount);
    printf("Reserved      : %hhu\n", ic_dir[i].bReserved);
    printf("Color Planes  : %hu\n", ic_dir[i].wPlanes);
    printf("Bit Count     : %hu\n", ic_dir[i].wBitCount);
    printf("Bytes in Res  : %u\n", ic_dir[i].dwBytesInRes);
    printf("Image Offset  : %#x\n", ic_dir[i].dwImageOffset);

    read_bmphdr(f, (long)ic_dir[i].dwImageOffset, argv[1]);
  } 

  fclose(f);

  return 0;
}

void read_bmphdr(FILE* f, long offset, char* filename) {

  BITMAPINFOHEADER hdr; 

  if (fseek(f, offset, SEEK_SET)) {
    error(1, errno, "In read_bmphdr: offset: %ld", offset);
  }
  
  if (fread(&hdr, 1, BMIH_SZ, f) != BMIH_SZ) {
      fferror(f, filename);
  }
  if (hdr.biSize == PNG) {
    puts("PNG Image - Not handled");
    return;
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

}

void read_colortbl(FILE* f, ICONDIRENTRY* direntry, BITMAPINFOHEADER* hdr, 
    char* filename) {
  /* Try to identify color table (palette) entries between the end of the 
   * BITMAPINFOHEADER and the beginning of the Image. The beginning of the 
   * colortbl est à direntry->dwImageOffset + BMIH_SZ.
   * The end is at direntry->dwImageOffset + direntry->dwBytesInRes -
   * hdr->biSizeImage.
   */

}

arguments parseoptions(int argc, char** argv) {

  int optchar; // Option character returned by getopt
  arguments args; 

              //
  while ((optchar = getopt(argc, argv, "x:") != -1)) {

    switch (optchar) {

      case 'x': 
        args.fileno = atoi(optarg);
        break;

    }
  }

  if (strlen(argv[optind]) >  MAXFILENAMELENGTH) {
    fprintf(stderr, "Filename `%s` too long\n", argv[optind]);
    exit(1);
  }

  strcpy(args.filename, argv[optind]);

  return args;

}
