#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <stdbool.h>
void init_graphics();
void draw(unsigned char *video_mem);
void st_flash(bool is_pixel_on);

#endif
