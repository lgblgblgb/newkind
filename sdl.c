/**
 *
 * Elite - The New Kind.
 *
 * SDL2 related routines, largely based on the original Allegro version of Graphics routines.
 * In fact, kinda hacky, trying to emulate Allegro at some placed in odd ways ...
 *
 * The code in this file has not been derived from the original Elite code.
 * Written by C.J.Pinder 1999-2001.
 * email: <christian@newkind.co.uk>
 * This code is re-worked/extened by G.Lenart (LGB) 2019, <lgblgblgb@gmail.com>
 *
 * Routines for drawing anti-aliased lines and circles by T.Harte.
 *
 **/

#include "etnk.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <SDL.h>
#include "SDL2_gfxPrimitives.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "sdl.h"
#include "elite.h"
#include "keyboard.h"
#include "datafile.h"
#include "sound.h"
#include "file.h"

static SDL_Texture	*sdl_tex = NULL;
static SDL_Window	*sdl_win = NULL;
static SDL_Renderer	*sdl_ren = NULL;

#define MAX_POLYS	100

static int start_poly;
static int total_polys;

int have_joystick;

struct poly_data
{
	int z;
	int no_points;
	int face_colour;
	int point_list[16];
	int next;
};

static struct poly_data poly_chain[MAX_POLYS];


#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888


#define RGBA_PARAM(col)				the_palette_r[col],the_palette_g[col],the_palette_b[col],0xFF

// sdl2_gfx substitutions of allegro functions

#define rectfill(ren,tx,ty,bx,by,c)		boxRGBA(sdl_ren,tx,ty,bx,by,RGBA_PARAM(c))
#define	line(ren,x1,y1,x2,y2,c)			lineRGBA(sdl_ren,x1,y1,x2,y2,RGBA_PARAM(c))
#define hline(ren,x1,y,x2,c)			hlineRGBA(sdl_ren,x1,x2,y,RGBA_PARAM(c))
#define vline(ren,x1,y1,y2,c)			vlineRGBA(sdl_ren,x1,y1,y2,RGBA_PARAM(c))
#define circle(ren,x,y,r,c)			circleRGBA(sdl_ren,x,y,r,RGBA_PARAM(c))
#define circlefill(ren,x,y,r,c)			filledCircleRGBA(sdl_ren,x,y,r,RGBA_PARAM(c))
#define putpixel(ren,x,y,c)			pixelRGBA(sdl_ren,x,y,RGBA_PARAM(c))
#define triangle(ren,x1,y1,x2,y2,x3,y3,c)	filledTrigonRGBA(sdl_ren,x1,y1,x2,y2,x3,y3,RGBA_PARAM(c))

// #define textout(g,font,str,x,y,c)		fprintf(stderr,"FIXME: no string function (textout) for displaying string \"%s\" at pos %d,%d\n",str,x,y)
// #define textout_centre(g,font,str,x,y,c)	fprintf(stderr,"FIXME: no string function (textout_centre) for displaying string \"%s\" at pos %d,%d\n",str,x,y)

#define textout(g,font,str,x,y,c)		stringRGBA(sdl_ren,x,y,str,RGBA_PARAM(c))
#define textout_centre(g,font,str,x,y,c)	stringRGBA(sdl_ren,x-strlen(str)*4,y,str,RGBA_PARAM(c))


Uint8 the_palette_r[0x100];
Uint8 the_palette_g[0x100];
Uint8 the_palette_b[0x100];
//Uint32 the_palette32[0x100];

#ifdef RES_512_512
#	define	SCREEN_W	512
#	define	SCREEN_H	512
#else
#	define	SCREEN_W	800
#	define	SCREEN_H	600
#endif

static struct {
	SDL_Texture *tex;
	SDL_Rect rect;
//	int w;
//	int h;
} sprites[IMG_NUM_OF];


SDL_RWops *datafile_open ( const char *fn )
{
	const Uint8 *data_p;
	int data_size;
	datafile_select(fn, &data_p, &data_size);
	SDL_RWops *v = SDL_RWFromConstMem(data_p, data_size);
	if (!v) {
		ERROR_WINDOW("DATAFILE: cannot create rwmem from databank for \"%s\": %s", fn, SDL_GetError());
		exit(1);	// brutal ...
	}
	return v;
}



#define IS_IMG_EXTERNAL	0x100



static void load_sprite ( int i, const char *fn, SDL_Surface **pass_surface_back )
{
	SDL_Surface *surface;
	if (i & IS_IMG_EXTERNAL) {
		char path[PATH_MAX];
		i &= ~IS_IMG_EXTERNAL;
		sprintf(path, "%s%s", pref_path, fn);
		surface = SDL_LoadBMP(path);
		if (!surface) {
			printf("SPRITE: cannot load sprite %s: %s\n", path, SDL_GetError());
			return;
		}
	} else {
		surface = SDL_LoadBMP_RW(datafile_open(fn), 1);
		if (!surface) {
			ERROR_WINDOW("Cannot load \"%s\": %s", fn, SDL_GetError());
			exit(1);
		}
	}
	sprites[i].tex = SDL_CreateTextureFromSurface(sdl_ren, surface);
	if (!pass_surface_back)
		SDL_FreeSurface(surface);
	else
		*pass_surface_back = surface;
	if (!sprites[i].tex) {
		ERROR_WINDOW("Cannot create texture from \"%s\": %s", fn, SDL_GetError());
		exit(1);
	}
	if (SDL_QueryTexture(sprites[i].tex, NULL, NULL, &sprites[i].rect.w, &sprites[i].rect.h)) {
		ERROR_WINDOW("Cannot query texture for \"%s\": %s", fn, SDL_GetError());
		exit(1);
	}
	// these must be set by the drawer func ...
	sprites[i].rect.x = 0;
	sprites[i].rect.y = 0;
}



// Allegro fixed math stuffs to "emulate" ...
static ETNK_INLINE fixed itofix ( int x ) {
	return x << 16;
}
static ETNK_INLINE fixed ftofix ( double x ) {
	if (x > 32767.0) {
		//*allegro_errno = ERANGE;
		return 0x7FFFFFFF;
	}
	if (x < -32767.0) {
		//*allegro_errno = ERANGE;
		return -0x7FFFFFFF;
	}
	return (fixed)(x * 65536.0 + (x < 0 ? -0.5 : 0.5));
}
static ETNK_INLINE double fixtof ( fixed x ) {
	return (double)x / 65536.0;
}
static ETNK_INLINE fixed fixmul ( fixed x, fixed y ) {
	return ftofix(fixtof(x) * fixtof(y));
#if 0
	LONG_LONG lx = x;
	LONG_LONG ly = y;
	LONG_LONG lres = (lx*ly);
	if (lres > 0x7FFFFFFF0000LL) {
		*allegro_errno = ERANGE;
		return 0x7FFFFFFF;
	} else if (lres < -0x7FFFFFFF0000LL) {
		*allegro_errno = ERANGE;
		return 0x80000000;
	} else {
		int res = lres >> 16;
		return res;
	}
#endif
}
static ETNK_INLINE fixed fixdiv ( fixed x, fixed y ) {
	if (y == 0) {
		//*allegro_errno = ERANGE;
		return (x < 0) ? -0x7FFFFFFF : 0x7FFFFFFF;
	} else
		return ftofix(fixtof(x) / fixtof(y));
}
#define fmul(x,y)	fixmul(x,y)
#define fdiv(x,y)	fixdiv(x,y)



int gfx_graphics_startup (void)
{
	//PALETTE the_palette;
	//int rv;

	sdl_win = SDL_CreateWindow(
		OUR_WINDOW_TITLE,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_W, SCREEN_H,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS
	);
#if 0

#ifdef ALLEGRO_WINDOWS	
#ifdef RES_512_512
	rv = set_gfx_mode(GFX_DIRECTX_OVL, 512, 512, 0, 0);
	if (rv != 0)
		rv = set_gfx_mode(GFX_DIRECTX_WIN, 512, 512, 0, 0);

	if (rv != 0)
		rv = set_gfx_mode(GFX_GDI, 512, 512, 0, 0);

	if (rv == 0)
		set_display_switch_mode (SWITCH_BACKGROUND);
#else
 	rv = set_gfx_mode(GFX_DIRECTX, 800, 600, 0, 0);
	
	if (rv != 0)
		rv = set_gfx_mode(GFX_GDI, 800, 600, 0, 0);
#endif

#else
	rv = set_gfx_mode(prefer_window ?
			    GFX_AUTODETECT_WINDOWED : GFX_AUTODETECT,
			  800, 600, 0, 0);
#endif
#endif

//	if (rv != 0) {
	if (!sdl_win) {
		//set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		ERROR_WINDOW("Unable to open window: %s", SDL_GetError());
		return 1;
	}

	sdl_ren = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
	if (!sdl_ren) {
		ERROR_WINDOW("Cannot create renderer: %s", SDL_GetError());
		return 1;
	}
	SDL_RenderSetLogicalSize(sdl_ren, SCREEN_W, SCREEN_H);
	SDL_PixelFormat *pixfmt = SDL_AllocFormat(PIXEL_FORMAT);
	if (!pixfmt) {
		ERROR_WINDOW("Cannot allocate pixel format: %s", SDL_GetError());
		return 1;
	}

#if 0
	datafile = load_datafile("elite.dat");
	if (!datafile) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		ERROR_WINDOW("Error loading %s!\n", "elite.dat");
		return 1;
	}
#endif

#if 0
	scanner_image = load_bitmap(scanner_filename, the_palette);
	if (!scanner_image) {
		set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
		ERROR_WINDOW("Error reading scanner bitmap file.\n");
		return 1;
	}
#endif
	sdl_tex = SDL_CreateTexture(sdl_ren, PIXEL_FORMAT, SDL_TEXTUREACCESS_TARGET /*| SDL_TEXTUREACCESS_STREAMING */, SCREEN_W, SCREEN_H);
	if (!sdl_tex) {
		ERROR_WINDOW("Cannot create texture: %s", SDL_GetError());
		return 1;
	}
	if (SDL_SetRenderTarget(sdl_ren, sdl_tex)) {
		ERROR_WINDOW("Cannot set render target: %s", SDL_GetError());
		return 1;
	}
	SDL_SetRenderDrawColor(sdl_ren, 0,0,0,0xFF);
	SDL_RenderClear(sdl_ren);

	for (int a = 0; a < IMG_NUM_OF; a++)
		sprites[a].tex = NULL;

	SDL_Surface *surface = NULL;
	if (*scanner_filename) {
		printf("SCANNER: trying to use %s as the scanner\n", scanner_filename);
		load_sprite(IMG_THE_SCANNER | IS_IMG_EXTERNAL, scanner_filename, &surface);
	} else {
		puts("SCANNER: no external scanner was specified");
	}
	if (!surface) {
		puts("SCANNER: defaulting to built-in scanner ...");
		load_sprite(IMG_THE_SCANNER, "scanner.bmp",	&surface);
	}
	load_sprite(IMG_GREEN_DOT,	"greendot.bmp",	NULL);
	load_sprite(IMG_RED_DOT,	"reddot.bmp",	NULL);
	load_sprite(IMG_BIG_S,		"safe.bmp",	NULL);
	load_sprite(IMG_ELITE_TXT,	"elitetx3.bmp",	NULL);
	load_sprite(IMG_BIG_E,		"ecm.bmp",	NULL);
	load_sprite(IMG_BLAKE,		"blake.bmp",	NULL);
	load_sprite(IMG_MISSILE_GREEN,	"missgrn.bmp",	NULL);
	load_sprite(IMG_MISSILE_YELLOW,	"missyell.bmp",	NULL);
	load_sprite(IMG_MISSILE_RED,	"missred.bmp",	NULL);
//	surface = SDL_LoadBMP(scanner_filename);
//	if (!surface) {
//		ERROR_WINDOW("Cannot load scanner: %s", SDL_GetError());
//		return 1;
//	}
	for (int a = 0; a < 0x100; a++) {
		the_palette_r[a] = surface->format->palette->colors[a].r;
		the_palette_g[a] = surface->format->palette->colors[a].g;
		the_palette_b[a] = surface->format->palette->colors[a].b;
		//the_palette32[a] = SDL_MapRGBA(pixfmt, the_palette_r[a], the_palette_g[a], the_palette_b[a], 0xFF);
	}
	SDL_FreeSurface(surface);
//	scanner_texture = SDL_CreateTextureFromSurface(sdl_ren, surface);
//	scanner_texture = sprites[0].tex;

	surface = SDL_LoadBMP_RW(datafile_open("icon.bmp"), 1);
	if (surface) {
		SDL_SetWindowIcon(sdl_win, surface);
		SDL_FreeSurface(surface);
	}

	/* select the scanner palette */
	//set_palette(the_palette);

	/* Create the screen buffer bitmap */
	//gfx_screen = create_bitmap (SCREEN_W, SCREEN_H);
	

	//clear (gfx_screen);

	//blit (scanner_image, gfx_screen, 0, 0, GFX_X_OFFSET, 385+GFX_Y_OFFSET, scanner_image->w, scanner_image->h);
	sprites[IMG_THE_SCANNER].rect.x = GFX_X_OFFSET;	// unlike other "sprites" the position is the same to put, always, so set it here ...
	sprites[IMG_THE_SCANNER].rect.y = 385 + GFX_Y_OFFSET;
	//scanner_rect.x = GFX_X_OFFSET;
	//scanner_rect.y = 385+GFX_Y_OFFSET;
	//scanner_rect.w = sprites[IMG_THE_SCANNER].w;
	//scanner_rect.h = sprites[IMG_THE_SCANNER].h;
	//scanner_rect.w = scanner_texture->width;
	//scanner_rect.h = scanner_texture->height;
	//SDL_QueryTexture(scanner_texture,NULL,NULL,&scanner_rect.w,&scanner_rect.h);
	//SDL_RenderCopy(sdl_ren, sprites[IMG_THE_SCANNER].tex, NULL, &scanner_rect);
	//gfx_draw_scanner();
	SDL_RenderCopy(sdl_ren, sprites[IMG_THE_SCANNER].tex, NULL, &sprites[IMG_THE_SCANNER].rect);	// render scanner without setting clipping (would be with gfx_draw_scanner ...)
	gfx_draw_line (0, 0, 0, 384);
	gfx_draw_line (0, 0, 511, 0);
	gfx_draw_line (511, 0, 511, 384);
	//gfx_draw_scanner();


#if 0
	/* Install a timer to regulate the speed of the game... */

	LOCK_VARIABLE(frame_count);
	LOCK_FUNCTION(frame_timer);
	frame_count = 0;
	install_int (frame_timer, speed_cap);
#endif	
	return 0;
}


void gfx_graphics_shutdown (void)
{
	puts("ETNK: graphics shutdown");
#if 0
	destroy_bitmap(scanner_image);
	destroy_bitmap(gfx_screen);
	unload_datafile(datafile);
#endif
}


/*
 * Blit the back buffer to the screen.
 */

void gfx_update_screen (void)
{
#if 0
	while (frame_count < 1)
		rest (10);
	frame_count = 0;
	
	acquire_screen();
 	blit (gfx_screen, screen, GFX_X_OFFSET, GFX_Y_OFFSET, GFX_X_OFFSET, GFX_Y_OFFSET, 512, 512);
	release_screen();
#endif
	// TODO FIXME: add frame rate controll here?
	puts("gfx_update_screen() is called!");
	//SDL_RenderSetLogicalSize(sdl_ren, SCREEN_W, SCREEN_H);
	/* switch renderer to the actual target (window) and render output */
	SDL_SetRenderTarget(sdl_ren, NULL);
	SDL_SetRenderDrawColor(sdl_ren,0,0,0,0xFF);
	SDL_RenderClear(sdl_ren);
	SDL_RenderCopy(sdl_ren, sdl_tex, NULL, NULL);
	SDL_RenderPresent(sdl_ren);
	//handle_sdl_events();
	/* switch renderer target back to the texture then */
	SDL_SetRenderTarget(sdl_ren, sdl_tex);
	//SDL_SetRenderDrawColor(sdl_ren,0,0,0,0xFF);
	//SDL_RenderClear(sdl_ren);
	// FIXME:
	// more sane framerate control
    #ifdef __EMSCRIPTEN__
        emscripten_sleep(speed_cap);
    #else
        SDL_Delay(speed_cap);
    #endif
    //

}


void gfx_acquire_screen (void)
{
	// acquire_bitmap (gfx_screen);
	puts("FIXME: gfx_acquire_screen() is not implemented");
}


void gfx_release_screen (void)
{
	// release_bitmap(gfx_screen);
	puts("FIXME: gfx_release_screen() is not implemented");
}


void gfx_fast_plot_pixel (int x, int y, int col)
{
	/* _putpixel(gfx_screen, x, y, col); */
	//gfx_screen->line[y][x] = col;
	// FIXME really, it should be "FAST"?
	putpixel(whatever, x, y, col);

}


void gfx_plot_pixel (int x, int y, int col)
{
	putpixel (gfx_screen, x + GFX_X_OFFSET, y + GFX_Y_OFFSET, col);
}


void gfx_draw_filled_circle (int cx, int cy, int radius, int circle_colour)
{
	circlefill (gfx_screen, cx + GFX_X_OFFSET, cy + GFX_Y_OFFSET, radius, circle_colour);
}


#define AA_BITS 3
#define AA_AND  7
#define AA_BASE 235

#define trunc(x) ((x) & ~65535)
#define frac(x) ((x) & 65535)
#define invfrac(x) (65535-frac(x))
#define plot(x,y,c) putpixel(gfx_screen, (x), (y), (c)+AA_BASE)

/*
 * Draw anti-aliased wireframe circle.
 * By T.Harte.
 */

void gfx_draw_aa_circle(int cx, int cy, int radius)
{
	int x,y;
	int s;
	int sx, sy;

	cx += GFX_X_OFFSET;
	cy += GFX_Y_OFFSET;

	radius >>= (16 - AA_BITS);

	x = radius;
	s = -radius;
	y = 0;

	while (y <= x)
	{
		/* wide pixels */
		sx = cx + (x >> AA_BITS); sy = cy + (y >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx + 1,	sy,	x&AA_AND);

		sy = cy - (y >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx + 1,	sy,	x&AA_AND);

		sx = cx - (x >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx - 1,	sy,	x&AA_AND);

		sy = cy + (y >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx - 1,	sy,	x&AA_AND);

		/* tall pixels */
		sx = cx + (y >> AA_BITS); sy = cy + (x >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx,	sy + 1,	x&AA_AND);

		sy = cy - (x >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx,	sy - 1,	x&AA_AND);

		sx = cx - (y >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx,	sy - 1,	x&AA_AND);

		sy = cy + (x >> AA_BITS);

		plot(sx,	sy,	AA_AND - (x&AA_AND));
		plot(sx,	sy + 1,	x&AA_AND);

		s +=	AA_AND+1 + (y << (AA_BITS+1)) + ((1 << (AA_BITS+2))-2);
		y +=	AA_AND+1;

		while(s >= 0)
		{
			s -= (x << 1) + 2;
			x --;
		}
	}
}


/*
 * Draw anti-aliased line.
 * By T.Harte.
 */
 
void gfx_draw_aa_line (int x1, int y1, int x2, int y2)
{
	fixed grad, xd, yd;
	fixed xgap, ygap, xend, yend, xf, yf;
	fixed brightness1, brightness2, swap;

	int x, y, ix1, ix2, iy1, iy2;

	x1 += itofix(GFX_X_OFFSET);
	x2 += itofix(GFX_X_OFFSET);
	y1 += itofix(GFX_Y_OFFSET);
	y2 += itofix(GFX_Y_OFFSET);

	xd = x2 - x1;
	yd = y2 - y1;

	if (abs(xd) > abs(yd))
	{
		if(x1 > x2)
		{
			swap = x1; x1 = x2; x2 = swap;
			swap = y1; y1 = y2; y2 = swap;
			xd   = -xd;
			yd   = -yd;
		}

		grad = fdiv(yd, xd);

		/* end point 1 */

		xend = trunc(x1 + 32768);
		yend = y1 + fmul(grad, xend-x1);

		xgap = invfrac(x1+32768);

		ix1  = xend >> 16;
		iy1  = yend >> 16;

		brightness1 = fmul(invfrac(yend), xgap);
		brightness2 = fmul(frac(yend), xgap);

		plot(ix1, iy1, brightness1 >> (16-AA_BITS));
		plot(ix1, iy1+1, brightness2 >> (16-AA_BITS));

		yf = yend+grad;

		/* end point 2; */

		xend = trunc(x2 + 32768);
		yend = y2 + fmul(grad, xend-x2);

		xgap = invfrac(x2 - 32768);

		ix2 = xend >> 16;
		iy2 = yend >> 16;

		brightness1 = fmul(invfrac(yend), xgap);
		brightness2 = fmul(frac(yend), xgap);
      
		plot(ix2, iy2, brightness1 >> (16-AA_BITS));
		plot(ix2, iy2+1, brightness2 >> (16-AA_BITS));

		for(x = ix1+1; x <= ix2-1; x++)
		{
			brightness1 = invfrac(yf);
			brightness2 = frac(yf);

			plot(x, (yf >> 16), brightness1 >> (16-AA_BITS));
			plot(x, 1+(yf >> 16), brightness2 >> (16-AA_BITS));

			yf += grad;
		}
	}
	else
	{
		if(y1 > y2)
		{
			swap = x1; x1 = x2; x2 = swap;
			swap = y1; y1 = y2; y2 = swap;
			xd   = -xd;
			yd   = -yd;
		}

		grad = fdiv(xd, yd);

		/* end point 1 */

		yend = trunc(y1 + 32768);
		xend = x1 + fmul(grad, yend-y1);

		ygap = invfrac(y1+32768);

		iy1  = yend >> 16;
		ix1  = xend >> 16;

		brightness1 = fmul(invfrac(xend), ygap);
		brightness2 = fmul(frac(xend), ygap);

		plot(ix1, iy1, brightness1 >> (16-AA_BITS));
		plot(ix1+1, iy1, brightness2 >> (16-AA_BITS));

		xf = xend+grad;

		/* end point 2; */

		yend = trunc(y2 + 32768);
		xend = x2 + fmul(grad, yend-y2);

		ygap = invfrac(y2 - 32768);

		ix2 = xend >> 16;
		iy2 = yend >> 16;

		brightness1 = fmul(invfrac(xend), ygap);
		brightness2 = fmul(frac(xend), ygap);
      
		plot(ix2, iy2, brightness1 >> (16-AA_BITS));
		plot(ix2+1, iy2, brightness2 >> (16-AA_BITS));

		for(y = iy1+1; y <= iy2-1; y++)
		{
			brightness1 = invfrac(xf);
			brightness2 = frac(xf);

			plot((xf >> 16), y, brightness1 >> (16-AA_BITS));
			plot(1+(xf >> 16), y, brightness2 >> (16-AA_BITS));

			xf += grad;
		}
	}
}

#undef trunc
#undef frac
#undef invfrac
#undef plot

#undef AA_BITS
#undef AA_AND
#undef AA_BASE



void gfx_draw_circle (int cx, int cy, int radius, int circle_colour)
{
	puts("gfx_draw_circle()");
	//circle (gfx_screen, cx + GFX_X_OFFSET, cy + GFX_Y_OFFSET, radius, circle_colour);

//#if 0

	if (anti_alias_gfx && (circle_colour == GFX_COL_WHITE))
		gfx_draw_aa_circle (cx, cy, itofix(radius));
	else	
		circle (gfx_screen, cx + GFX_X_OFFSET, cy + GFX_Y_OFFSET, radius, circle_colour);
//#endif
}



void gfx_draw_line (int x1, int y1, int x2, int y2)
{
	if (y1 == y2)
	{
		hline (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, GFX_COL_WHITE);
		return;
	}

	if (x1 == x2)
	{
		vline (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, y2 + GFX_Y_OFFSET, GFX_COL_WHITE);
		return;
	}

	if (anti_alias_gfx)
		gfx_draw_aa_line (itofix(x1), itofix(y1), itofix(x2), itofix(y2));
	else
		line (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, GFX_COL_WHITE);
}



void gfx_draw_colour_line (int x1, int y1, int x2, int y2, int line_colour)
{
	if (y1 == y2)
	{
		hline (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, line_colour);
		return;
	}

	if (x1 == x2)
	{
		vline (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, y2 + GFX_Y_OFFSET, line_colour);
		return;
	}

	if (anti_alias_gfx && (line_colour == GFX_COL_WHITE))
		gfx_draw_aa_line (itofix(x1), itofix(y1), itofix(x2), itofix(y2));
	else
		line (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET, line_colour);
}



void gfx_draw_triangle (int x1, int y1, int x2, int y2, int x3, int y3, int col)
{
	triangle (gfx_screen, x1 + GFX_X_OFFSET, y1 + GFX_Y_OFFSET, x2 + GFX_X_OFFSET, y2 + GFX_Y_OFFSET,
				   x3 + GFX_X_OFFSET, y3 + GFX_Y_OFFSET, col);
}



void gfx_display_text (int x, int y, char *txt)
{
	//text_mode (-1);
	textout (gfx_screen, datafile[ELITE_1].dat, txt, (x / (2 / GFX_SCALE)) + GFX_X_OFFSET, (y / (2 / GFX_SCALE)) + GFX_Y_OFFSET, GFX_COL_WHITE);
}


void gfx_display_colour_text (int x, int y, char *txt, int col)
{
	//text_mode (-1);
	textout (gfx_screen, datafile[ELITE_1].dat, txt, (x / (2 / GFX_SCALE)) + GFX_X_OFFSET, (y / (2 / GFX_SCALE)) + GFX_Y_OFFSET, col);
}



void gfx_display_centre_text (int y, char *str, int psize, int col)
{
	int txt_colour;
#if 0
	// FIXME: add txt_size support!
	int txt_size;
	
	if (psize == 140)
	{
		txt_size = ELITE_2;
		txt_colour = -1;
	}
	else
	{
		txt_size = ELITE_1;
		txt_colour = col;
	}
#endif
	txt_colour = col;
	//text_mode (-1);
	textout_centre (gfx_screen,  datafile[txt_size].dat, str, (128 * GFX_SCALE) + GFX_X_OFFSET, (y / (2 / GFX_SCALE)) + GFX_Y_OFFSET, txt_colour);
}


void gfx_clear_display (void)
{
	rectfill (gfx_screen, GFX_X_OFFSET + 1, GFX_Y_OFFSET + 1, 510 + GFX_X_OFFSET, 383 + GFX_Y_OFFSET, GFX_COL_BLACK);
}

void gfx_clear_text_area (void)
{
	rectfill (gfx_screen, GFX_X_OFFSET + 1, GFX_Y_OFFSET + 340, 510 + GFX_X_OFFSET, 383 + GFX_Y_OFFSET, GFX_COL_BLACK);
}


void gfx_clear_area (int tx, int ty, int bx, int by)
{
	rectfill (gfx_screen, tx + GFX_X_OFFSET, ty + GFX_Y_OFFSET,
				   bx + GFX_X_OFFSET, by + GFX_Y_OFFSET, GFX_COL_BLACK);
}




void gfx_draw_rectangle (int tx, int ty, int bx, int by, int col)
{
	rectfill (gfx_screen, tx + GFX_X_OFFSET, ty + GFX_Y_OFFSET,
				   bx + GFX_X_OFFSET, by + GFX_Y_OFFSET, col);
}


void gfx_display_pretty_text (int tx, int ty, int bx, int by, char *txt)
{
	char strbuf[100];
	char *str;
	char *bptr;
	int len;
	int pos;
	int maxlen;
	
	maxlen = (bx - tx) / 8;

	str = txt;	
	len = strlen(txt);
	
	while (len > 0)
	{
		pos = maxlen;
		if (pos > len)
			pos = len;

		while ((str[pos] != ' ') && (str[pos] != ',') &&
			   (str[pos] != '.') && (str[pos] != '\0'))
		{
			pos--;
		}

		len = len - pos - 1;
	
		for (bptr = strbuf; pos >= 0; pos--)
			*bptr++ = *str++;

		*bptr = '\0';

		//text_mode (-1);
		textout (gfx_screen, datafile[ELITE_1].dat, strbuf, tx + GFX_X_OFFSET, ty + GFX_Y_OFFSET, GFX_COL_WHITE);
		ty += (8 * GFX_SCALE);
	}
}


static ETNK_INLINE void set_clip ( int x1, int y1, int x2, int y2 )
{
	SDL_Rect rect;
	rect.x = x1;
	rect.y = y1;
	rect.w = x2 - x1 + 1;
	rect.h = y2 - y1 + 1;
	// FIXME: check?
	if (rect.w <= 0 || rect.h <=0 || rect.x < 0 || rect.y < 0)
		fprintf(stderr, "SUSPECT clipping: set_clip(%d,%d,%d,%d)\n", x1,y1,x2,y2);
	SDL_RenderSetClipRect(sdl_ren, &rect);
}




void gfx_draw_scanner (void)
{
	//fprintf(stderr, "FIXME: gfx_draw_scanner() is not implemented :(\n");
	set_clip(/*gfx_screen,*/ GFX_X_OFFSET, 385 + GFX_Y_OFFSET,
		 GFX_X_OFFSET + sprites[IMG_THE_SCANNER].rect.w,
		 GFX_Y_OFFSET + sprites[IMG_THE_SCANNER].rect.h + 385);

	SDL_RenderCopy(sdl_ren, sprites[IMG_THE_SCANNER].tex, NULL, &sprites[IMG_THE_SCANNER].rect);
#if 0
	set_clip(/*gfx_screen,*/ GFX_X_OFFSET, 385 + GFX_Y_OFFSET,
		 GFX_X_OFFSET + scanner_image->w,
		 GFX_Y_OFFSET + scanner_image->h + 385);
	blit (scanner_image, gfx_screen, 0, 0, GFX_X_OFFSET,
	     385+GFX_Y_OFFSET, scanner_image->w, scanner_image->h);
#endif
}

void gfx_set_clip_region (int tx, int ty, int bx, int by)
{
	set_clip (/*gfx_screen,*/ tx + GFX_X_OFFSET, ty + GFX_Y_OFFSET, bx + GFX_X_OFFSET, by + GFX_Y_OFFSET);
}


void gfx_start_render (void)
{
	start_poly = 0;
	total_polys = 0;
}


void gfx_render_polygon (int num_points, int *point_list, int face_colour, int zavg)
{
	int i;
	int x;
	int nx;
	
	if (total_polys == MAX_POLYS)
		return;

	x = total_polys;
	total_polys++;
	
	poly_chain[x].no_points = num_points;
	poly_chain[x].face_colour = face_colour;
	poly_chain[x].z = zavg;
	poly_chain[x].next = -1;

	for (i = 0; i < 16; i++)
		poly_chain[x].point_list[i] = point_list[i];				

	if (x == 0)
		return;

	if (zavg > poly_chain[start_poly].z)
	{
		poly_chain[x].next = start_poly;
		start_poly = x;
		return;
	} 	

	for (i = start_poly; poly_chain[i].next != -1; i = poly_chain[i].next)
	{
		nx = poly_chain[i].next;
		
		if (zavg > poly_chain[nx].z)
		{
			poly_chain[i].next = x;
			poly_chain[x].next = nx;
			return;
		}
	}	
	
	poly_chain[i].next = x;
}


void gfx_render_line (int x1, int y1, int x2, int y2, int dist, int col)
{
	int point_list[4];
	
	point_list[0] = x1;
	point_list[1] = y1;
	point_list[2] = x2;
	point_list[3] = y2;
	
	gfx_render_polygon (2, point_list, col, dist);
}


void gfx_finish_render (void)
{
	int num_points;
	int *pl;
	int i;
	int col;
	
	if (total_polys == 0)
		return;
		
	for (i = start_poly; i != -1; i = poly_chain[i].next)
	{
		num_points = poly_chain[i].no_points;
		pl = poly_chain[i].point_list;
		col = poly_chain[i].face_colour;

		if (num_points == 2)
		{
			gfx_draw_colour_line (pl[0], pl[1], pl[2], pl[3], col);
			continue;
		}
		
		gfx_polygon (num_points, pl, col); 
	};
}



void gfx_polygon (int num_points, int *poly_list, int face_colour)
{
#if 0
	int i;
	int x,y;
	
	x = 0;
	y = 1;
	for (i = 0; i < num_points; i++)
	{
		poly_list[x] += GFX_X_OFFSET;
		poly_list[y] += GFX_Y_OFFSET;
		x += 2;
		y += 2;
	}
	
	polygon (gfx_screen, num_points, poly_list, face_colour);
#endif
	Sint16 vx[MAX_POLYS], vy[MAX_POLYS];
	for (int i = 0, j = 0; i < num_points; i++) {
		vx[i] = poly_list[j++] + GFX_X_OFFSET;
		vy[i] = poly_list[j++] + GFX_Y_OFFSET;
	}
	filledPolygonRGBA(sdl_ren, vx, vy, num_points, RGBA_PARAM(face_colour));
}


void gfx_draw_sprite ( int sprite_no, int x, int y )
{
	if (sprite_no >= IMG_NUM_OF || !sprites[sprite_no].tex) {
		ERROR_WINDOW("gfx_draw_sprite(): trying to render non-existing sprite number #%d", sprite_no);
		exit(1);
	}
	if (x == -1)
		x = ((256 * GFX_SCALE) - sprites[sprite_no].rect.w) / 2;
	//draw_sprite (gfx_screen, sprite_bmp, x + GFX_X_OFFSET, y + GFX_Y_OFFSET);
	//SDL_Rect rect;
	sprites[sprite_no].rect.x = x + GFX_X_OFFSET;
	sprites[sprite_no].rect.y = y + GFX_Y_OFFSET;
	//rect.w = sprites[sprite_no].w;
	//rect.h = sprites[sprite_no].h;
	SDL_RenderCopy(sdl_ren, sprites[sprite_no].tex, NULL, &sprites[sprite_no].rect);
}


int gfx_request_file (char *title, char *path, char *ext)
{
	// TODO / FIXME
	fprintf(stderr, "FIXME: add file selector code! [title=\"%s\" path=\"%s\" ext=\"%s\"]\n", title, path, ext);
	return 0;
#if 0
	int okay;

	show_mouse (screen);
	okay = file_select (title, path, ext);
	show_mouse (NULL);

	return okay;
#endif
}



static void shutdown_sdl ( void )
{
	puts("SDL: shutting system down ...");
	snd_sound_shutdown();
	if (sdl_tex)
		SDL_DestroyTexture(sdl_tex);
	for (int i = 0; i < IMG_NUM_OF; i++)
		if (sprites[i].tex)
			SDL_DestroyTexture(sprites[i].tex);
	if (sdl_ren)
		SDL_DestroyRenderer(sdl_ren);
	if (sdl_win)
		SDL_DestroyWindow(sdl_win);
	SDL_Quit();
}


int init_sdl ( void )
{
    printf("hello, world!\n");

    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init() failed: %s\n", SDL_GetError());
        return 1;
    }
//	if (SDL_Init(
//#ifdef __EMSCRIPTEN__
		/* It seems there is an issue with emscripten SDL2: SDL_Init does not work if TIMER and/or HAPTIC is tried to be intialized or just "EVERYTHING" is used!! */
//		SDL_INIT_EVERYTHING & ~(SDL_INIT_TIMER | SDL_INIT_HAPTIC)
//        SDL_Init(SDL_INIT_VIDEO)
//#else
//		SDL_INIT_EVERYTHING
//#endif
//	)) {
//		ERROR_WINDOW("Cannot initialize SDL: %s", SDL_GetError());
//		return 1;
//	}
	atexit(shutdown_sdl);
//	pref_path = SDL_GetPrefPath("lgb", "newkind");
//	if (!pref_path) {
//		ERROR_WINDOW("Cannot make use of pref path: %s", SDL_GetError());
//		return 1;
//	}
	//SDL_StartTextInput();
#if 0
	allegro_init();
	install_keyboard(); 
	install_timer();
	install_mouse();
#endif
	// FIXME: no joystick ...
	have_joystick = 0;
#if 0   
	if (install_joystick(JOY_TYPE_AUTODETECT) == 0) {
		have_joystick = (num_joysticks > 0);
	}
#endif
	return 0;
}

int sdl_last_key_pressed;
char key[KEY_MAX];


static const struct {
	SDL_Keycode sdl;
	int etnk;
} keydefs[] = {
	{ SDLK_0, KEY_0 },
	{ SDLK_a, KEY_a },
	{ SDLK_BACKSPACE, KEY_BACKSPACE },
	{ SDLK_c, KEY_c },
	{ SDLK_COMMA, KEY_COMMA },
	{ SDLK_d, KEY_d },
	{ SDLK_DOWN, KEY_DOWN },
	{ SDLK_e, KEY_e },
	{ SDLK_ESCAPE, KEY_ESCAPE },
	{ SDLK_f, KEY_f },
	{ SDLK_F1, KEY_F1 },
	{ SDLK_F10, KEY_F10 },
	{ SDLK_F11, KEY_F11 },
	{ SDLK_F12, KEY_F12 },
	{ SDLK_F2, KEY_F2 },
	{ SDLK_F3, KEY_F3 },
	{ SDLK_F4, KEY_F4 },
	{ SDLK_F5, KEY_F5 },
	{ SDLK_F6, KEY_F6 },
	{ SDLK_F7, KEY_F7 },
	{ SDLK_F8, KEY_F8 },
	{ SDLK_F9, KEY_F9 },
	{ SDLK_h, KEY_h },
	{ SDLK_i, KEY_i },
	{ SDLK_j, KEY_j },
	{ SDLK_LCTRL, KEY_LCTRL },
	{ SDLK_LEFT, KEY_LEFT },
	{ SDLK_m, KEY_m },
	{ SDLK_n, KEY_n },
	{ SDLK_o, KEY_o },
	{ SDLK_p, KEY_p },
	{ SDLK_r, KEY_r },
	{ SDLK_RCTRL, KEY_RCTRL },
	{ SDLK_RETURN, KEY_RETURN },
	{ SDLK_RIGHT, KEY_RIGHT },
	{ SDLK_s, KEY_s },
	{ SDLK_SLASH, KEY_SLASH },
	{ SDLK_SPACE, KEY_SPACE },
	{ SDLK_STOP, KEY_STOP },
	{ SDLK_t, KEY_t },
	{ SDLK_TAB, KEY_TAB },
	{ SDLK_u, KEY_u },
	{ SDLK_UP, KEY_UP },
	{ SDLK_x, KEY_x },
	{ SDLK_y, KEY_y },
	{ SDLK_z, KEY_z },
	{ 0, 0 }
};


static int decode_keysym ( SDL_Keycode sym )
{
	for (int i = 0; keydefs[i].sdl; i++) {
		if (sym == keydefs[i].sdl)
			return keydefs[i].etnk;
	}
	return -1;
}


void handle_sdl_events ( void )
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				exit(0);	// FIXME: do it nicer ....
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				printf("KEY: scan=%s[#%d] sym=%s[#%d] event=%s repeated=%d\n",
					SDL_GetScancodeName(event.key.keysym.scancode),
					event.key.keysym.scancode,
					SDL_GetKeyName(event.key.keysym.sym),
					event.key.keysym.sym,
					event.key.state == SDL_PRESSED ? "DOWN": "UP",
					event.key.repeat
				);
				if (!event.key.repeat) {
					int game_code = decode_keysym(event.key.keysym.sym);
					//printf("KEY_MAP=%d\n", game_code);
					if (game_code >= 0) {
						if (event.key.state == SDL_PRESSED) {
							key[game_code] = 1;
							sdl_last_key_pressed = game_code;
						} else {
							key[game_code] = 0;
						}
					} /*else {
						puts("KEY IS NOT MAPPED");
					} */
				}
				break;
		}
	}
}



