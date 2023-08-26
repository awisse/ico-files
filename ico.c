/* vim:set sts=2:sw=2: */
// A short programm to analyze the header of an .ico file
#define _XOPEN_SOURCE
#include "ico.h"

int main(int argc, char** argv) {

  FILE* f;
  arguments args;
  ICONDIR iconhdr;
  ICONDIRENTRY *ic_dir, *direntry;
  BITMAPINFOHEADER bmih;
  size_t dirbytes; // Length of ic_dir in bytes
  int errorflag; // Return value of functions. No error if zero.
  u_int16_t fileno;

  args = parseargs(argc, argv);

  if ((f = fopen(args.filename, "r")) == NULL) {
    error(1, errno, "Can't open file %s\n", args.filename);
    exit(1);
  }

  if (fread(&iconhdr, 1, IDIR_SZ, f) != IDIR_SZ) {
    fferror(f, "File header");
  }

  if (iconhdr.idType != 1) {
    if (iconhdr.idType != 2) 
      fferror(f, "Not an icon file");
    fferror(f, "Icon ID type");
  }

  dirbytes = iconhdr.idCount * IDEN_SZ;
  ic_dir = (ICONDIRENTRY*)malloc(dirbytes);

  if (fread(ic_dir, 1, dirbytes, f) != dirbytes) {
    fferror(f, "ICONDIR");
  }

  fileno = args.fileno;
  if (fileno == MAXHW) {
    printf("Number of images: %u\n", iconhdr.idCount);
    // Print all directory entries
    for (u_int16_t i=0; i<iconhdr.idCount; i++) {
      direntry = &ic_dir[i];
      print_direntry(direntry);

      errorflag = read_bmphdr(f, (long)direntry->dwImageOffset, &bmih);
      switch (errorflag) {
        case -1: 
          fferror(f, "BITMAPINFOHEADER");
        case 1:
          continue;
      }
    } 
  } else if (fileno > iconhdr.idCount) {
    fprintf(stderr, "Image number %hu too high\n", fileno);
  } else {
    printf("\nImage %d\n", fileno);
    direntry = &ic_dir[fileno];
    print_direntry(direntry);

    errorflag = read_bmphdr(f, (long)direntry->dwImageOffset, &bmih);
    switch (errorflag) {
      case -1: 
        fferror(f, "BITMAPINFOHEADER");
      case 1:
        fclose(f);
        exit(0);
    }
      
    // The following elements are presented only for one bitmap in the file
    // There is only a colortable for less than 8 bits of color
    if ((args.colortable) & (bmih.biBitCount <= 8)) {
      if (read_colortbl(f, &bmih)) 
        fferror(f, "colortable");
    }

    if (args.showand) {
      if (showand(f, direntry, &bmih)) 
        fferror(f, "seeking AND mask");
    }

    if (args.extract) {
      if (extract(f, direntry, &args, "bmp")) 
        fferror(f, "extract .bmp file");
    }
  } 

  fclose(f);

  return 0;
}

void print_direntry(ICONDIRENTRY* direntry) {

    printf("Width x Height: %d x %d\n", 
      zero256(direntry->bWidth), zero256(direntry->bWidth));
    printf("Color Count   : %hhu\n", direntry->bColorCount);
    printf("Reserved      : %hhu\n", direntry->bReserved);
    printf("Color Planes  : %hu\n", direntry->wPlanes);
    printf("Bit Count     : %hu\n", direntry->wBitCount);
    printf("Bytes in Res  : %u\n", direntry->dwBytesInRes);
    printf("Image Offset  : %#x\n", direntry->dwImageOffset);
}

int read_bmphdr(FILE* f, long offset, BITMAPINFOHEADER* hdr) {

  if (fseek(f, offset, SEEK_SET)) {
    error(1, errno, "In read_bmphdr: offset: %ld", offset);
    return -1;
  }
  
  if (fread(hdr, 1, BMIH_SZ, f) != BMIH_SZ) {
      return -1;
  }

  if (hdr->biSize == PNG) {
    puts("PNG Image - Not handled");
    return 1;
  }

  puts("Bitmap Info:");
  printf("biSize          %u\n", hdr->biSize);
  printf("biWidth         %d\n", hdr->biWidth);
  printf("biHeight        %d\n", hdr->biHeight);
  printf("biPlanes        %hu\n", hdr->biPlanes);
  printf("biBitCount      %hu\n", hdr->biBitCount);
  printf("biCompression   %u\n", hdr->biCompression);
  printf("biSizeImage     %u\n", hdr->biSizeImage);
  printf("biXPelsPerMeter %d\n", hdr->biXPelsPerMeter);
  printf("biYPelsPerMeter %d\n", hdr->biYPelsPerMeter);
  printf("biClrUsed       %u\n", hdr->biClrUsed);
  printf("biClrImportant  %u\n", hdr->biClrImportant);

  return 0;
}

int read_colortbl(FILE* f, BITMAPINFOHEADER* hdr) {
  /* Read and display colortable if there is one */
  u_int32_t i, color, num_colors = 1 << hdr->biBitCount;

  puts("Color table:");
  for (i=0; i<num_colors; i++) {
    if (fread(&color, 1, 4, f) < 4)
      break;
    printf("Color %3u: %2X %2X %2X\n", i, xcolor(color, RED), 
        xcolor(color, GREEN), xcolor(color, BLUE));
  }


  return 0;
}

arguments parseargs(int argc, char** argv) {

  int optchar; // Option character returned by getopt
  arguments args; 

  if (argc < 2) {
    fprintf(stderr, "No argument provided. Filename is mandatory.\n");
    helpmsg();
    exit(1);
  }

  // reset args
  memset(&args, 0, sizeof(arguments));
  args.fileno = MAXHW; // Default: show all entries without detail

  while ((optchar = getopt(argc, argv, "achn:x")) != -1) {
    switch (optchar) {
      case 'a':
        args.showand = TRUE;
        break;
      case 'c':
        args.colortable = TRUE;
        break;
      case 'h':
        helpmsg();
        exit(0);
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

  if (argv[optind] == 0) {
    fputs("Filename is mandatory.\n", stderr);
    helpmsg();
    exit(1);
  }

  if (strlen(argv[optind]) >  MAXFILENAMELENGTH) {
    fprintf(stderr, "Filename `%s` too long\n", argv[optind]);
    exit(1);
  }

  strcpy(args.filename, argv[optind]);

  return args;
}

int showand(FILE* f, ICONDIRENTRY* direntry, BITMAPINFOHEADER* hdr) {
  /* Show the AND bitmask as ASCII art */
  long clr_tbl_size = 0; // Size of color table
  long image_size; // Size of image in bytes
  long and_map_pos; // Position of AND map in file
  u_int8_t width = direntry->bWidth, height = direntry->bHeight;
  u_int16_t clr_bits = direntry->wBitCount;

  // Position the file at the beginning of the AND mask
  image_size = width * height * clr_bits / 8;
  clr_tbl_size = clr_bits <= 8 ? (1 << clr_bits) * 4 : 0;
  and_map_pos = direntry->dwImageOffset + hdr->biSize + clr_tbl_size 
    + image_size;

  if (fseek(f, and_map_pos, SEEK_SET) == -1) {
    return -1;
  }

  // TODO: Read the AND mask.
  return 0;
}

int extract(FILE* f, ICONDIRENTRY* idir, arguments* args, char* extension) {

  // TODO: Extract only the image, not the AND mask
  FILE* sf;
  char* savefile;
  char* extpos; // position of last dot in filename
  u_int8_t* buffer; // Read/write buffer
  size_t bmp_size = idir->dwBytesInRes; // Length of bitmap data
  char bm[2] = "BM";
  BITMAPFILEHEADER bmp_fheader;
  BITMAPINFOHEADER *bmp_header;

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
    return 1;
  }

  if (strcmp(extension, "bmp") == 0) {
    // Set the BITMAPFILEHEADER
    memset(&bmp_fheader, 0, BMPFH_SZ);
    bmp_fheader.size = bmp_size + BMPFH_SZ + 2;
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
    // TODO correct the BITMAPINFOHEADER: Drop the AND mask
    bmp_header = (BITMAPINFOHEADER*)buffer;
    bmp_header->biHeight = idir->bHeight;

    fwrite(buffer, bmp_size, 1, sf);
    free(buffer);
    fclose(sf);

    printf("Image %d written to %s\n", args->fileno, savefile);
  }

  free(savefile);

  return 0;
}

void helpmsg() {
  
  puts("Usage:   readico (or ri-db) [options] <.ico file>\n\n"
       "  -h     This help message.\n"
       "  -n <N> Select image number N.\n"
       "  -x     Extract image selected with -n.\n"
       "  -c     Print colortable of selected image.\n"
       "  -a     Print AND bitmap in ASCII art of selected image.\n");
}

