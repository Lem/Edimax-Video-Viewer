/*
* SDL-part of the edimax video-util
* alpha 0.02
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
+
* authors:
* (C) 2010  m0x <unkown>
*
*/


#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdio.h>
#include <string.h>

SDL_Surface *screen;

int init_display(int x, int y)
{
  int res = SDL_Init(SDL_INIT_VIDEO);
  if(res)
  {
    printf("can't init SDL: %s\n", SDL_GetError());
    return -1;
  }

  screen = SDL_SetVideoMode(x, y, 24, SDL_DOUBLEBUF|SDL_HWSURFACE);
  /*change to SDL_SWSURFACE and remove SDL_DOUBELBUF if hw is not available*/
  if(!screen)
  {
    printf("can't open window: %s\n", SDL_GetError());
    return -1;
  }
  
  if(IMG_Init(IMG_INIT_JPG) != IMG_INIT_JPG)
  {
    printf("can't init jpeg laoder: %s\n", SDL_GetError());
    return -1;
  }

  return 0;
}

int close_display()
{
  SDL_FreeSurface(screen);
  IMG_Quit();
  SDL_Quit();
  return 0;
}

static int blit_surface(SDL_Surface* s)
{
  return SDL_BlitSurface(s, NULL, screen, NULL);
}

int show_jpegmem(void* ptr, unsigned long len)
{
  SDL_RWops *c;
  SDL_Surface *s;
  int res = 0;

  c= SDL_RWFromMem(ptr, len);
  if(!c)
  {
    printf("can't alloc RWops: %s", SDL_GetError());
    return -1;
  }

  if(!(s = IMG_LoadJPG_RW(c)))
  {
    printf("can't load from mem: %s\n", SDL_GetError());
    SDL_FreeRW(c);
    return -1;
  }

  if(blit_surface(s)) res = -1;

  SDL_FreeSurface(s);
  SDL_FreeRW(c);
  return res;
}

int show_jpegfile(const char * file)
{
  int res = 0;
  SDL_Surface *s;
  s = IMG_Load(file);
  if(!s)
  {
    printf("can't load: %s, %s\n", file, SDL_GetError());
    return -1;
  }

  if(blit_surface(s)) res=-1;

  SDL_FreeSurface(s);
  return res;
}

int update_display()
{
  SDL_Flip(screen);
  SDL_Event e;
  while(SDL_PollEvent(&e))
  {
    switch (e.type)
    {
      case SDL_QUIT: /*SIG INT*/
        return -1;
        break;
      default:
        //printf("event %i not implemented!\n", e.type);
        break;
    }
  }
  return 0;
}


