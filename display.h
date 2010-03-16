/*
* SDL-part of the edimax video-util
* alpha 0.01
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
+
* authors:
* (C) 2010  m0x <unkown>
*
*/


#ifndef DISPLAY_H
#define DISPLAY_H

/*
  -1 = error
  0  = success
  for every function listed here
*/


int init_display(int x, int y);
int update_display();

/*
  Cleanup code
*/
int close_display();

/*
  creates an image from memory and draws it (less expensive)
*/
int show_jpegmem(void* ptr, unsigned long len);

/*
  Reloads the file and draws it everytime you call this (expensive)
*/
int show_jpegfile(const char*file); 


#endif
