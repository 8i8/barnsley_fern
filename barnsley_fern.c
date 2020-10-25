#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>

typedef struct {
	SDL_Window *mWindow;
	SDL_Renderer *mScreen;
	int screen_w;
	int screen_h;
	int full_screen;
	int *map;
	int run;
	int draw;
	int pause;
	long unsigned i;
	double xc;
	double yc;
	double x;
	double y;
} Window;

typedef struct {
	SDL_Texture *mTexture;
	int texture_w;
	int texture_h;
} LTexture;

Window window;
LTexture mCount_texture;

SDL_sem* mGraphLock = NULL;
SDL_sem* mRenderLock = NULL;
TTF_Font *mFont = NULL;

short Text_load_surface(Window *w, LTexture *tx, char *string, SDL_Color text_color)
{
	SDL_Surface *temp_surface = TTF_RenderText_Solid(
						mFont, string, text_color);
	if (temp_surface == NULL) {
		SDL_SetError("Failed to create temp_surface surface.");
		SDL_Log("error: %s(): %s", __func__, SDL_GetError());
		return -1;
	}

	tx->mTexture = SDL_CreateTextureFromSurface(w->mScreen, temp_surface);

	if (tx->mTexture == NULL) {
		SDL_SetError("Failed to create tx->Text texture.");
		SDL_Log("error: %s(): %s", __func__, SDL_GetError());
		return -1;
	}

	tx->texture_w = temp_surface->w;
	tx->texture_h = temp_surface->h;

	SDL_FreeSurface(temp_surface);

	return 0;
}

short init(Window *w)
{
	w->mWindow = NULL;
	w->mScreen = NULL;
	w->screen_w = 1280;
	w->screen_h = 720;
	w->full_screen = 0;
	w->map = NULL;
	w->run = 1;
	w->draw = 1;
	w->i = 0;
	w->xc = 0;
	w->yc = 0;
	w->x = 0;
	w->y = 0;

	if (SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("%s() error: %s", __func__, SDL_GetError());
		return -1;
	}

	if(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1") == 0)
		return SDL_SetError("%s() Linear texture filtering not enabled.",
				__func__);

	mFont = TTF_OpenFont("DejaVuSansMono.ttf", 28);

	w->mWindow = SDL_CreateWindow("Barnsley's Fern multithread",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			w->screen_w,
			w->screen_h,
			SDL_WINDOW_SHOWN);

	if (w->mWindow == NULL) {
		SDL_SetError("failed to create mWindow.");
		SDL_Log("%s() error: %s", __func__, SDL_GetError());
		return -1;
	}

	w->mScreen = SDL_CreateRenderer(
			w->mWindow,
			-1,
			SDL_RENDERER_SOFTWARE);

	if (w->mScreen == NULL) {
		SDL_SetError("failed to create mScreen.");
		SDL_Log("%s() error: %s", __func__, SDL_GetError());
		return -1;
	}

	mGraphLock = SDL_CreateSemaphore(1);
	mRenderLock = SDL_CreateSemaphore(1);

	if(TTF_Init() < 0) {
		SDL_Log("%s() error: %s", __func__, SDL_GetError());
		return -1;
	}

	return 0;
}

short LTexture_render(
			Window *w,
			LTexture *lt,
			int x,
			int y,
			SDL_Rect* clip)
{
	SDL_Rect renderQuad = { x, y, lt->texture_w, lt->texture_h };

	if(clip != NULL) {
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	return SDL_RenderCopy(w->mScreen, lt->mTexture, clip, &renderQuad);
}

void Graph_plot(Window *w, int x, int y, int z)
{
	int i, j;
	y = ~(y-w->screen_h);

	if (z > 1)
		for (i = -z/2; i < z/2+1; i++)
			for (j = 0; j < z; j++)
				SDL_RenderDrawPoint(w->mScreen, x+i, y+j);
	else
		SDL_RenderDrawPoint(w->mScreen, x, y);
}

void reset_screen(Window *w)
{
	free(w->map);
	w->map = calloc((w->screen_h*w->screen_w), sizeof(int*));
	SDL_SetRenderDrawColor(w->mScreen, 0x1F, 0x1F, 0x1F, 0x1F);
	SDL_RenderClear(w->mScreen);
	SDL_SetRenderDrawColor(w->mScreen, 0x00, 0xFF, 0x00, 0x00);
	SDL_RenderPresent(w->mScreen);
}

void close_all(Window *w)
{
	free(w->map);
	SDL_DestroyWindow(w->mWindow);

	SDL_DestroySemaphore(mGraphLock);
	SDL_DestroySemaphore(mRenderLock);
	mGraphLock = NULL;
	mRenderLock = NULL;

	SDL_Quit();
}

short key_down(Window *w, SDL_Event *e)
{
	switch (e->key.keysym.sym) {
		case SDLK_q:
			w->draw = 0;
			return 1;
		case SDLK_p:
			(w->draw) ? w->draw-- : w->draw++;
		default:
			break;
	}
	return 0;
}

short key_up(Window *w, SDL_Event *e)
{
	switch (e->key.keysym.sym)
	{
	}
	return 0;
}

short get_input(Window *w, SDL_Event *e)
{
	while (SDL_PollEvent(e) != 0)
	{
		switch (e->window.event)
		{
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				w->draw = 0;
				w->screen_w = e->window.data1;
				w->screen_h = e->window.data2;
				reset_screen(w);
				break;
			default:
				break;
		}
		switch(e->type)
		{
			case SDL_QUIT:
				w->draw = 0;
				return 1;
			case SDL_KEYDOWN:
				return key_down(w, e);
			case SDL_KEYUP:
				return key_up(w, e);
			default:
				break;
		}
	}
	SDL_Delay(500);
	return 0;
}

void Graph_fern(Window *w)
{
	int dice, x, y;
	dice = x = y = 0;

	srand(SDL_GetTicks());
	dice = rand()%100;

	if (dice == 0) {
		w->y = 0;
		w->x = 0.16*w->xc;
	}
	else if (dice >= 1 && dice <= 7) {
		w->y = -0.16*w->yc + 0.28*w->xc;
		w->x = 0.26*w->yc + 0.24*w->xc + 0.44;
	}
	else if (dice >= 8 && dice <= 15) {
		w->y = 0.2*w->yc - 0.26*w->xc;
		w->x = 0.23*w->yc + 0.22*w->xc + 1.6;
	}
	else {
		w->y = 0.85*w->yc + 0.04*w->xc;
		w->x = -0.04*w->yc + 0.85*w->xc + 1.6;
	}

	x = round(w->screen_w/11.0*w->x);
	y = round(w->screen_w/11.0*w->y + w->screen_h/2.0);

	if (*(w->map+(y * w->screen_w + x)) == 0) {
		Graph_plot(w, x, y, 1);
		w->i++;
		*(w->map+(y * w->screen_w + x)) = 1;
	}

	w->xc = w->x;
	w->yc = w->y;
}

/*
 * (x * w->screen_w + y)
 */
void Graph_draw(Window *w)
{
	int x, y;
	for (x = 0; x < w->screen_w; x++)
		for (y = 0; y < w->screen_h; y++)
			if (*(w->map+(y * w->screen_w + x)) == 1)
				Graph_plot(w, x, y, 1);
}

int thread(void* w)
{
	Window *t;
	t = (Window*)w;
//	SDL_Color text_color = { 0, 255, 0, 255 };
//	char count_text[25];

	while (t->run) {
		while (t->draw) {
			SDL_SemWait(mGraphLock);
			Graph_fern((Window*)w);
			SDL_SemPost(mGraphLock);

			SDL_SemWait(mRenderLock);
	//		sprintf(count_text, "%lu", t->i);
	//		if (Text_load_surface(t, &mCount_texture, count_text, text_color))
	//			return 0;
	//		LTexture_render(
	//				t,
	//				&mCount_texture,
	//				20,
	//				20,
	//				NULL);
			SDL_RenderPresent(t->mScreen);
			SDL_SemPost(mRenderLock);
		}
		SDL_Delay(500);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	SDL_Event e;

	init(&window);

	reset_screen(&window);
	SDL_Thread* thread01 = SDL_CreateThread(thread, "Thread 01", (void*)&window);
	SDL_Thread* thread02 = SDL_CreateThread(thread, "Thread 02", (void*)&window);
	SDL_Thread* thread03 = SDL_CreateThread(thread, "Thread 03", (void*)&window);
	SDL_Thread* thread04 = SDL_CreateThread(thread, "Thread 04", (void*)&window);
	/* SDL_Thread* thread05 = SDL_CreateThread(thread, "Thread 05", (void*)&window); */
	/* SDL_Thread* thread06 = SDL_CreateThread(thread, "Thread 06", (void*)&window); */
	/* SDL_Thread* thread07 = SDL_CreateThread(thread, "Thread 07", (void*)&window); */
	/* SDL_Thread* thread08 = SDL_CreateThread(thread, "Thread 08", (void*)&window); */
	/* SDL_Thread* thread09 = SDL_CreateThread(thread, "Thread 09", (void*)&window); */
	/* SDL_Thread* thread10 = SDL_CreateThread(thread, "Thread 10", (void*)&window); */
	/* SDL_Thread* thread11 = SDL_CreateThread(thread, "Thread 11", (void*)&window); */
	/* SDL_Thread* thread12 = SDL_CreateThread(thread, "Thread 12", (void*)&window); */
	/* SDL_Thread* thread13 = SDL_CreateThread(thread, "Thread 13", (void*)&window); */
	/* SDL_Thread* thread14 = SDL_CreateThread(thread, "Thread 14", (void*)&window); */
	/* SDL_Thread* thread15 = SDL_CreateThread(thread, "Thread 15", (void*)&window); */
	/* SDL_Thread* thread16 = SDL_CreateThread(thread, "Thread 16", (void*)&window); */

	while (window.run)
		if (get_input(&window, &e))
			window.run = 0;

	SDL_WaitThread(thread01, NULL);
	SDL_WaitThread(thread02, NULL);
	SDL_WaitThread(thread03, NULL);
	SDL_WaitThread(thread04, NULL);
	/* SDL_WaitThread(thread05, NULL); */
	/* SDL_WaitThread(thread06, NULL); */
	/* SDL_WaitThread(thread07, NULL); */
	/* SDL_WaitThread(thread08, NULL); */
	/* SDL_WaitThread(thread09, NULL); */
	/* SDL_WaitThread(thread10, NULL); */
	/* SDL_WaitThread(thread11, NULL); */
	/* SDL_WaitThread(thread12, NULL); */
	/* SDL_WaitThread(thread13, NULL); */
	/* SDL_WaitThread(thread14, NULL); */
	/* SDL_WaitThread(thread15, NULL); */
	/* SDL_WaitThread(thread16, NULL); */

	close_all(&window);

	return 0;
}

