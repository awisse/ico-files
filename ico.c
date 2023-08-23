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
#define MAXFILENO 256

typedef struct {
  int           extract; // Extract: 1. No extract: 0.
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
  fprintf(stderr, "Not a valid .ico file: %s\n", filename);
  fclose(f);
  exit(1);
}

void memerror(char* msg) {
  fprintf(stderr, "Memory allocation error: %s", msg);
}

arguments parseargs(int argc, char** argv);
int zero256(u_int8_t u) { return u ? u : 256; }
void extract(FILE* f, ICONDIRENTRY* idir, arguments* args, char* extension);
void read_bmphdr(FILE* f, long offset, char* filename); 
void read_colortbl(FILE* f, ICONDIRENTRY* direntry, BITMAPINFOHEADER* hdr,
        char* filename);

int main(int argc, char** argv) {

  FILE* f;
  arguments args;
  ICONDIR iconhdr;
  ICONDIRENTRY* ic_dir;
  size_t dirbytes; // Length of ic_dir in bytes

  args = parseargs(argc, argv);

  if ((f = fopen(args.filename, "r")) == NULL) {
    error(1, errno, "Can't open file %s\n", args.filename);
  }

  if (fread(&iconhdr, 1, IDIR_SZ, f) != IDIR_SZ) {
    fferror(f, args.filename);
  }

  if (iconhdr.idType != 1) {
    fferror(f, args.filename);
  }

  printf("Number of images: %u\n", iconhdr.idCount);

  dirbytes = iconhdr.idCount * IDEN_SZ;
  ic_dir = (ICONDIRENTRY*)malloc(dirbytes);

  if (fread(ic_dir, 1, dirbytes, f) != dirbytes) {
    fferror(f, args.filename);
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
    if ((i == args.fileno) & args.extract) {
      extract(f, &ic_dir[i], &args, "bmp");
    }
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

arguments parseargs(int argc, char** argv) {

  int optchar; // Option character returned by getopt
  arguments args; 

  if (argc < 2) {
    fprintf(stderr, "No argument provided. Filename is mandatory.\n");
    exit(1);
  }

  memset(&args, 0, sizeof(arguments));

  while ((optchar = getopt(argc, argv, "xn:")) != -1) {
    switch (optchar) {
      case 'n': 
        args.fileno = atoi(optarg);
        break;
      case 'x': 
        args.extract = 1;
        break;
      default:
        fprintf(stderr, "Unrecognized option -%c. Ignored", optchar);
    }
  }

  if (args.fileno > MAXFILENO) {
    fprintf(stderr, "FILENO must be smaller than %d\n", MAXFILENO);
    exit(1);
  }

  if (argv[optind] == 0) {
    fputs("Filename is mandatory.\n", stderr);
    exit(1);
  }

  if (strlen(argv[optind]) >  MAXFILENAMELENGTH) {
    fprintf(stderr, "Filename `%s` too long\n", argv[optind]);
    exit(1);
  }

  strcpy(args.filename, argv[optind]);

  return args;
}

void extract(FILE* f, ICONDIRENTRY* idir, arguments* args, char* extension) {

  FILE* sf;
  char* savefile;
  char* extpos; // position of last dot in filename
  u_int8_t* buffer; // Read/write buffer
  size_t bmp_size = idir->dwBytesInRes; // Length of bitmap data
  char bm[2] = "BM";
  BITMAPFILEHEADER bmp_fheader;

  // Create new filename: add -<fileno>.bmp to basename
  if ((savefile = (char*)malloc(strlen(args->filename) + 4)) == NULL) {
    memerror("extract - savefile");
    exit(1);
  }
  strcpy(savefile, args->filename);
  extpos = strrchr(savefile, '.');
  sprintf(extpos, "-%hu.%s", args->fileno, extension);

  printf("Save filename: %s\n",  savefile);

  if ((sf = fopen(savefile, "w")) == NULL) {
    fprintf(stderr, "Could not save to file %s\n", savefile);
    free(savefile);
    return;
  }

  if (strcmp(extension, "bmp") == 0) {
    // Set the BITMAPFILEHEADER
    memset(&bmp_fheader, 0, BMPFH_SZ);
    bmp_fheader.size = bmp_size + BMPFH_SZ;
    // Compute bitmap image data offset
    bmp_fheader.offset = 0x36; // BMPFH (0x0E) + BMPIH (0x28)

    // Write the BITMAPFILEHEADER to the file
    fwrite(&bm, 2, 1, sf);
    fwrite(&bmp_fheader, BMPFH_SZ, 1, sf);

    // Allocate memory for buffer
    if ((buffer = (u_int8_t*)malloc(bmp_size)) == NULL) {
      memerror("extract - buffer");
      fclose(sf);
      free(savefile);
      exit(1);
    }

    // Write the bitmap data to the file
    fseek(f, idir->dwImageOffset, SEEK_SET);
    fread(buffer, bmp_size, 1, f);
    fwrite(buffer, bmp_size, 1, sf);
    free(buffer);
    fclose(sf);

    printf("Image %d written to %s\n", args->fileno, savefile);
  }

  free(savefile);
}
