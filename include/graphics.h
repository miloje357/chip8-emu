#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <stdbool.h>
void init_graphics();
void draw(unsigned char *video_mem, unsigned int video_signal);
void clear_screen();
void st_flash(bool is_pixel_on);
void handle_xset_message();
void pattern();

#endif
