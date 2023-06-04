#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <errno.h>
#include <math.h>
#include <png.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "drw/drw.h"
#include "drw/util.h"

#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))


// Config
static const char col_black[]       = "#000000";
static const char col_gray[]       = "#444444";
static const char col_main[]        = "#ddf00a";
static const char *colors[][3]      = {
	/* fg         bg        border   */
	{  col_black, col_main, col_main },
	{  col_main,  col_main, col_main },
	{  col_gray,  col_main, col_gray },
};

// Globals

Drw *drw;
Clr *blk;
Clr *ylw;
XImage *image;


void close_x() {
	Display *dpy = drw->dpy;
	Window win = drw->root;
	drw_free(drw);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);

	exit(0);
}


void draw() {
	static int f = 0;
	f++;

	unsigned width, height;
	width = DisplayWidth(drw->dpy, drw->screen);
	height = DisplayHeight(drw->dpy, drw->screen);


	// Put frame on screen
	drw_map(drw, drw->root, 0, 0, width, height);

	// Clear pixmap
	drw_setscheme(drw, blk);
	drw_rect(drw, 0, 0, width, height, 1, 0);
}

void draw_selection_rect(int sx, int sy, int ex, int ey) {
	int tx = MIN(sx, ex);
	int ty = MIN(sy, ey);
	int w = MAX(sx, ex) - tx;
	int h = MAX(sy, ey) - ty;

	if (w == 0 || h == 0) return;

	drw_setscheme(drw, ylw);
	drw_rect(drw, tx, ty, w, h, 0, 0);
}


void xinit() {
	Display *dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		die("XOpenDisplay returned NULL");
	}

	int screen = DefaultScreen(dpy);

	Window root = DefaultRootWindow(dpy);

	unsigned long black, white;
	black = BlackPixel(dpy, screen);
	white = WhitePixel(dpy, screen);

	unsigned width, height;
	width = DisplayWidth(dpy, screen);
	height = DisplayHeight(dpy, screen);

	const int x = 0;
	const int y = 0;

	XSetWindowAttributes swa;
	swa.override_redirect = True;
	swa.event_mask = KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	Window win = XCreateWindow(dpy, root, x, y, width, height, 0,
			CopyFromParent, CopyFromParent, CopyFromParent,
			CWOverrideRedirect | CWEventMask, &swa);

	XSetStandardProperties(dpy, win, "ebin-bubbl", "ebin-bubbl", None, NULL, 0, NULL);

	// Init suckless drw library
	drw = drw_create(dpy, screen, win, width, height);

	XClearWindow(drw->dpy, drw->root);
	XMapRaised(drw->dpy, drw->root);

	XSelectInput(drw->dpy, drw->root, swa.event_mask);
	XGrabKeyboard(drw->dpy, drw->root, GrabModeSync, 1, GrabModeSync, CurrentTime);

	blk = drw_scm_create(drw, colors[0], 3);
	ylw = drw_scm_create(drw, colors[1], 3);
	drw_setscheme(drw, blk);
}

void take_screenshot() {
	Display *dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		die("NU DÖR JAG");
	}

	int screen = DefaultScreen(dpy);
	Window root = DefaultRootWindow(dpy);

	unsigned width, height;
	width = DisplayWidth(dpy, screen);
	height = DisplayHeight(dpy, screen);

	image = XGetImage(dpy, root, 0, 0, width, height, AllPlanes, ZPixmap);
}

void save_image(int crop_x, int crop_y, int crop_w, int crop_h) {
	// https://stackoverflow.com/questions/8249669/how-do-take-a-screenshot-correctly-with-xlib
	int code = 0;
	FILE *fp;
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_bytep png_row;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	// Create file
	char filepath[255] = "";
	char *homedir = getenv("HOME");
	strncat(filepath, homedir, sizeof(filepath));
	int len = strlen(filepath);
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	// YYMMDD-hhmmss is a very cool format i think
	snprintf(filepath + len, sizeof(filepath) - len, "/Images/screenshots/%02d%02d%02d-%02d%02d%02d.png", tm.tm_year-100, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fp = fopen(filepath, "wb");
	if (fp == NULL) die("Couldn't open output file");

	// Init libpng
	if (png_ptr == NULL) die("init png_ptr failed");
	png_info_ptr = png_create_info_struct(png_ptr);
	if (png_info_ptr == NULL) die("init png_info_ptr failed");
	if (setjmp(png_jmpbuf(png_ptr))) die("png backwards longjump error");
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, png_info_ptr, crop_w, crop_h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,PNG_FILTER_TYPE_BASE);

	// Set png info
	png_text title_text;
	title_text.compression = PNG_TEXT_COMPRESSION_NONE;
	title_text.key = "Screenyshot";
	title_text.text = "Capturated by ebin-scrot";
	png_write_info(png_ptr, png_info_ptr);

	// Write png to file
	png_row = (png_bytep) malloc(3*crop_w*sizeof(png_byte));
	unsigned long red_mask = image->red_mask;
	unsigned long green_mask = image->green_mask;
	unsigned long blue_mask = image->blue_mask;
	int x, y;
	for (y = 0; y < crop_h; y++) {
		for (x = 0; x < crop_w; x++) {
			unsigned long pixel = XGetPixel(image, x + crop_x, y + crop_y);
			unsigned char blue  = pixel & blue_mask;
			unsigned char green = (pixel & green_mask) >> 8;
	 		unsigned char red   = (pixel & red_mask) >> 16;
			png_byte *ptr = &(png_row[x*3]);
			ptr[0] = red;
			ptr[1] = green;
			ptr[2] = blue;
		}
		png_write_row(png_ptr, png_row);
	}
	png_write_end(png_ptr, NULL);

	// Copy image to clipboard
	if (fork() == 0) {
		execl("/usr/bin/xclip", "/usr/bin/xclip", "-selection", "clipboard", "-t", "image/png", "-i", filepath, NULL);
		return;
	}

	// Cleanup
	fclose(fp);
	if (png_info_ptr != NULL) png_free_data(png_ptr, png_info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (png_row != NULL) free(png_row);
}

void loop() {
	XEvent event;
	KeySym key;
	char text[255];

	int mouse_down = 0;
	int start_x = 0;
	int start_y = 0;
	int end_x = 0;
	int end_y = 0;

	while (1) {
		while (XPending(drw->dpy)) {
			XNextEvent(drw->dpy, &event);

			if (event.type == Expose || event.type == NoExpose) continue;

			if (event.type == KeyPress) {
				XLookupString(&event.xkey, text, 255, &key, 0);
			} else if (event.type == ButtonPress) {
				XButtonEvent xbe = event.xbutton;

				mouse_down = 1;
				start_x = xbe.x;
				start_y = xbe.y;
				end_x   = xbe.x;
				end_y   = xbe.y;
			} else if (event.type == ButtonRelease) {
				XButtonEvent xbe = event.xbutton;

				mouse_down = 0;

				int tx = MIN(start_x, end_x);
				int ty = MIN(start_y, end_y);
				int w = MAX(start_x, end_x) - tx;
				int h = MAX(start_y, end_y) - ty;

				save_image(tx, ty, w, h);
				close_x();
			} else if (event.type == MotionNotify) {
				XMotionEvent xme = event.xmotion;

				end_x = xme.x;
				end_y = xme.y;
			}
		}

		unsigned width, height;
		width = DisplayWidth(drw->dpy, drw->screen);
		height = DisplayHeight(drw->dpy, drw->screen);
		XPutImage(drw->dpy, drw->drawable, drw->gc, image, 0, 0, 0, 0, width, width);

		if (mouse_down)
			draw_selection_rect(start_x, start_y, end_x, end_y);
		draw();
		usleep(10000);
	}
}

void save_entire_image() {
	unsigned width, height;
	width = DisplayWidth(drw->dpy, drw->screen);
	height = DisplayHeight(drw->dpy, drw->screen);
	save_image(0, 0, width, height);
}

void usage() {
	puts("\"O ill-starred equality!\" No, my good old sir, nothing of\n"
			"equality. We only want to count for what we are worth, and,\n"
			"if you are worth more, you shall count for more right along.\n\n"
            "                 - The Ego and his Own (1844) out of context");
}

void main(int argc, char *argv[]) {
	take_screenshot();
	xinit();

	if (argc > 1) {
		if (!strcmp(argv[1], "all")) {
			save_entire_image();
		} else if (!strcmp(argv[1], "selection")) {
			loop();
		} else {
			usage();
		}
	} else {
		usage();
	}

	close_x();
}

