// SDL2 output plugin.

// Matrix order and size
#if !defined(MATRIX_ORDER_PLAIN) && !defined(MATRIX_ORDER_SNAKE)
#define MATRIX_ORDER_SNAKE // cause that's what i have, it's also the easiest to wire, IMO.
#endif

#ifndef MATRIX_X
#error Define MATRIX_X as the matrixes X size.
#endif

#ifndef MATRIX_Y
#error Define MATRIX_Y as the matrixes Y size.
#endif

#include <types.h>
#include <string.h>
#include <assert.h>
#include <timers.h>

// Calculation for amount of bytes needed.
#include <SDL2/SDL.h>

// SDL-based stuff, we need to create a buffer.
static size_t BUFFER_SIZE;
static RGB* BUFFER;

static int sdl_event_break;

#define WIN_W (MATRIX_X * SDL_SCALE_FACTOR)
#define WIN_H (MATRIX_Y * SDL_SCALE_FACTOR)

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_Rect dest = { .x = 0, .y = 0, .w = WIN_W, .h = WIN_H };

static int matx = MATRIX_X;
static int maty = MATRIX_Y;

int init(int modno, char *argstr) {
	if (SDL_Init(SDL_INIT_VIDEO))
		return 2;

	sdl_event_break = SDL_RegisterEvents(1);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
#ifdef SDLFULLSCREEN
	SDL_DisplayMode dispmode;
	if (SDL_GetCurrentDisplayMode(0, &dispmode) != 0)
		return 2;

	window = SDL_CreateWindow("sled: DEBUG Platform", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dispmode.w, dispmode.h, SDL_WINDOW_FULLSCREEN_DESKTOP);
	int ww = 0;
	int wh = 0;
	while (ww == 0 || wh == 0)
		SDL_GetWindowSize(window, &ww, &wh);
	dest.w = ww;
	dest.h = wh;
	matx = ww / SDL_SCALE_FACTOR;
	maty = wh / SDL_SCALE_FACTOR;
#else
	window = SDL_CreateWindow("sled: DEBUG Platform", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_W, WIN_H, 0);
#endif
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, matx, maty);

	BUFFER_SIZE = matx * maty * sizeof(RGB);
	BUFFER = malloc(BUFFER_SIZE);
	printf("buffer size of %i and %i = %zu\n", matx, maty, BUFFER_SIZE);
	assert(BUFFER);

	memset(BUFFER, 0, BUFFER_SIZE);

	return 0;
}


int getx(void) {
	return matx;
}
int gety(void) {
	return maty;
}

static int matrix_ppos(int x, int y) {
	return (x + (y * matx));
}

int set(int x, int y, RGB color) {
	// Detect OOB access.
	assert(x >= 0);
	assert(y >= 0);
	assert(x < matx);
	assert(y < maty);

	int pos = matrix_ppos(x, y);
	BUFFER[pos] = color;
	return 0;
}

RGB get(int x, int y) {
	// Detect OOB access.
	assert(x >= 0);
	assert(y >= 0);
	assert(x < matx);
	assert(y < maty);

	int pos = matrix_ppos(x, y);
	return BUFFER[pos];
}

// Zeroes the stuff.
int clear(void) {
	memset(BUFFER, 0, BUFFER_SIZE);
	return 0;
}

int render(void) {
	SDL_UpdateTexture(texture, NULL, BUFFER, matx * 4);
	SDL_RenderCopy(renderer, texture, NULL, &dest);
	SDL_RenderPresent(renderer);
	return 0;
}

ulong wait_until(ulong desired_usec) {
	SDL_Event ev;
	while (1) {
		ulong tnow = udate();
		if (tnow >= desired_usec)
			return tnow;

		int sleeptimems = (desired_usec - tnow) / 1000;
		if (SDL_WaitEventTimeout(&ev, sleeptimems)) {
			if (ev.type == SDL_QUIT) {
				timers_doquit();
				return udate();
			} else if (ev.type == sdl_event_break) {
				wait_until_break_cleanup_core();
				return udate();
			}
		} else {
			return wait_until_core(desired_usec);
		}
	}
}

void wait_until_break(void) {
	SDL_Event myevent;
	memset(&myevent, 0, sizeof(myevent));
	myevent.type = sdl_event_break;
	SDL_PushEvent(&myevent);
	wait_until_break_core();
}

int deinit(void) {
	// Destroy everything.
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	free(BUFFER);
	return 0;
}
