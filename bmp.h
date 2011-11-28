/* $Author: cs442 $ $Revision: 1.1 $ $Date: 2001/11/19 20:08:24 $ */

/*
 * File: bmp.h
 * Author: Wayne O. Cochran  (cochran@vancouver.wsu.edu) with
 * Purpose:
 *   Interface for the routine bmpGetImage() which reads a Windows BMP
 *   file. The stored image is particulary suited for using as
 *   a texture map.
 * Modification History:
 *   11/19/2001 (WOC) ............... created.
 */

#ifndef BMP_H
#define BMP_H

typedef struct {            /* Image read from BMP image */
  short w, h;               /* original width and height of image */
  int W, H;                 /* actual width and height of image */ 
  float smax, tmax;         /* valid texture map coords in (0,0)-(smax,tmax)*/
  unsigned char (*rgba)[4]; /* red/green/blue/alpha pixel values */  
} BmpImage;

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
void bmpGetImage(char *fname, BmpImage *image, int padToPowerOf2);

#endif /* BMP_H */
