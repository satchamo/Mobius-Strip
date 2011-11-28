/* $Author: cs442 $ $Revision: 1.1 $ $Date: 2001/11/19 20:08:09 $ */

/*
 * File: bmp.c
 * Purpose: Provide MS Windows BMP file I/O. Initially designed to only
 *   read 24-bit BMp files.
 * Author: Wayne O. Cochran  (cochran@vancouver.wsu.edu) with
 *   code heavily lifted from the appendix of
 *           Computer Graphics Using Open GL, Second Edition 
 *           by F.S. Hill, Jr 
 *   http://cwx.prenhall.com/bookbind/pubbooks/hill4/chapter0/deluxe.html
 * Modification History:
 *   11/19/2001 (WOC) ............... created.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp.h"

/*
 * Read 32-bit integer from file stored little-endian style.
 */
static int get32(FILE *f) {
  unsigned long num;
  unsigned char bytes[4];
  fread(bytes, 1, 4, f);  
  num = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
  return (int) num;
}

/*
 * Read 16-bit integer from file stored little-endian style.
 */
static int get16(FILE *f) {
  unsigned short num;
  unsigned char bytes[2];
  fread(bytes, 1, 2, f);  
  num = (bytes[1] << 8) | bytes[0];
  return (int) num;
}

/*
 * Finds the smallest value p such that
 * p = 2^k and p >= n.
 */
static int nextPowerOf2(int n) {
  int pow;
  for (pow = 1; pow < n; pow <<= 1)
    ;
  return pow;
}  

/*
 * bmpGetImage();
 * Purpose: Read 24-bit Windows BMP file into image buffer with 4 bytes
 *   per pixel (RGBA). Size of image may be resized to powers of 2 
 *   in both width and height -- any added RGBA values are 0.
 * Parameters:
 *   fname: file name of 24-bit BMP file;
 *   image: ptr to structure to store image data;
 *   padToPowerOf2: 1 ==> pad image size to power of 2.
 * Note:
 *   The first pixel stored in a BMP file represents the
 *   lower left-hand portion of the image. This is nice, since
 *   the image (i,j) coordinate system has the same orientation
 *   as the texture map (s,t) coordinate system.
 */
void bmpGetImage(char *fname, BmpImage *image, int padToPowerOf2) {
  FILE *f;
  static char buf[200];
  int i, j;
  int fileSize, imageSize;
  int offset;
  int headerSize;
  int w, h, d, W, H;
  int bpp;
  int compression;
  int xpels, ypels;
  int lutSize;
  int impColors;
  unsigned char (*rgba)[4];

  if ((f = fopen(fname, "rb")) == NULL) {
    perror(fname);
    exit(-1);
  }

  /*
   * Check first two magic bytes of BMP file.
   */
  if (fread(buf, 1, 2, f) != 2 || strncmp("BM", buf, 2) != 0) {
    fprintf(stderr, "Bogus BMP file '%s'!\n", fname);
    exit(-1);
  }

  fileSize = get32(f);           /* file size */

  get16(f); get16(f);            /* two reserved words */

  offset = get32(f);             /* offset to image (unreliable) */
  headerSize = get32(f);         /* always 40 */
  w = get32(f); h = get32(f);    /* width and height of image */
  d = get16(f);                  /* always 1 */

  /*
   * Bits per pixel better be 24.
   */
  if ((bpp = get16(f)) != 24) {
    fprintf(stderr, "Sorry, '%s' not a 24-bit image!\n", fname);
    exit(-1);
  }

  /*
   * No compression.
   */
  if ((compression = get32(f)) != 0) {
    fprintf(stderr, "Sorry, '%s' is compressed!\n", fname);
    exit(-1);
  }

  imageSize = get32(f);         /* size of images */

  xpels = get32(f);             /* always 0 */
  ypels = get32(f);             /* always 0 */

  /*
   * No lookup table for 24-bit image (256 for 8-bit images).
   */
  if ((lutSize = get32(f)) != 0) {
    fprintf(stderr, "Bad LUT '%s'!\n", fname);
    exit(-1);
  }

  impColors = get32(f);         /* always 0 */

  /*
   * If requested, pad image buffer so its dimensions are powers of 2
   */
  if (padToPowerOf2) {
    W = nextPowerOf2(w);
    H = nextPowerOf2(h);
  } else {
    W = w;
    H = h;
  }

  /*
   * Allocate image buffer.
   */
  if ((rgba = (unsigned char (*)[4]) calloc(W*H, 4)) == NULL) {
    perror("calloc() rgba[]");
    exit(-1);
  }

  /*
   * Read in RGB values for each pixel (blue stored as high byte).
   * Alpha value of 255 used for fetched pixel. RGBA values are 0
   * for the rest of the image.
   */
  for (j = 0; j < h; j++)
    for (i = 0; i < w; i++) {
      unsigned char bgr[3];
      int index;
      if (fread(bgr, 1, 3, f) != 3) {
	if (ferror(f))
          perror(fname);
        else
          fprintf(stderr, "Unexpected EOF: %s\n", fname);
        exit(-1);
      }
      index = j*W + i;
      rgba[index][0] = bgr[2];
      rgba[index][1] = bgr[1];
      rgba[index][2] = bgr[0];
      rgba[index][3] = 255;
    }

  fclose(f);

  image->w = w;
  image->h = h;
  image->W = W;
  image->H = H;
  image->smax = ((float) w)/W;
  image->tmax = ((float) h)/H;
  image->rgba = rgba;
}

#ifdef TEST_BMP

/*
 * Creates upside down image!!!
 * Interesting. The first pixel must be the lower left hand corner,
 * *not* the upper left-hand corner -- this is actually kind of
 * nice for texture mapping, since the texture (s,t) coordinate system
 * has the same orientation as the image (i,j) coordinate system.
 */

int main(int argc, char *argv[]) {
  int i, j;
  FILE *f;
  BmpImage image;

  if (argc < 3) {
    fprintf(stderr,
	    "usage: %s <input-BMP-file-name> <output-PPM-file-name>\n", 
	    argv[0]);
    exit(1);
  }

  bmpGetImage(argv[1], &image, 1);

  if ((f = fopen(argv[2], "wb")) == NULL) {
    perror(argv[2]);
    exit(-1);
  }
  fprintf(f, "P6\n");
  fprintf(f, "# created from '%s' (%d, %d) (%d, %d) (%g, %g)\n",
	  argv[1], image.w, image.h, image.W, image.H, image.smax, image.tmax);
  fprintf(f, "%d %d\n", image.w, image.h);
  fprintf(f, "255\n");
  for (j = 0; j < image.h; j++)
    for (i = 0; i < image.w; i++)
      fwrite(image.rgba[j*image.W + i], 1, 3, f);
  fclose(f);

  return 0;
}

#endif /* TEST_BMP */
