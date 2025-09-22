#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// TODO(bruno): maybe go through this file replacing malloc with the proper
// syscall for this platform. maybe see the "allocating a backbuffer" episode of
// handmadehero to see how he does it on windows and replicate it over here.

#define internal static
#define global_variable static
#define local_persist static

#define X_EVENT_BUFFER_SIZE 255

#include "handmade.cpp"

struct Backbuffer
{
	Display *dis;
	int screen;
	Window win;
	GC gc;
	int width, height;
	uint32_t *pixels;
};

global_variable Backbuffer globalBackbuffer;

void InitializeBackbuffer(Backbuffer *backbuffer)
{
	/* get the colors black and white (see section for details) */
	unsigned long black, white;

	/* use the information from the environment variable DISPLAY
	   to create the X connection:
	*/
	backbuffer->dis = XOpenDisplay((char *)0);
	backbuffer->screen = DefaultScreen(backbuffer->dis);
	black =
		BlackPixel(backbuffer->dis, backbuffer->screen), /* get color black */
		white = WhitePixel(backbuffer->dis,
						   backbuffer->screen); /* get color white */

	backbuffer->width = 640;
	backbuffer->height = 480;
	backbuffer->pixels = (uint32_t *)malloc(
		backbuffer->width * backbuffer->height * sizeof(uint32_t));

	backbuffer->win = XCreateSimpleWindow(
		backbuffer->dis, DefaultRootWindow(backbuffer->dis), 0, 0,
		backbuffer->width, backbuffer->height, 5, white, black);

	/* here is where some properties of the window can be set.
	   The third and fourth items indicate the name which appears
	   at the top of the window and the name of the minimized window
	   respectively.
	*/
	XSetStandardProperties(backbuffer->dis, backbuffer->win, "Handmade Hero",
						   "HI!", None, NULL, 0, NULL);

	/* this routine determines which types of input are allowed in
	   the input.  see the appropriate section for details...
	*/
	XSelectInput(backbuffer->dis, backbuffer->win,
				 ExposureMask | ButtonPressMask | KeyPressMask |
					 ResizeRedirectMask);

	/* create the Graphics Context */
	backbuffer->gc = XCreateGC(backbuffer->dis, backbuffer->win, 0, 0);

	/* here is another routine to set the foreground and background
	   colors _currently_ in use in the window.
	*/
	XSetBackground(backbuffer->dis, backbuffer->gc, white);
	XSetForeground(backbuffer->dis, backbuffer->gc, black);

	/* clear the window and bring it on top of the other windows */
	XClearWindow(backbuffer->dis, backbuffer->win);
	XMapRaised(backbuffer->dis, backbuffer->win);
};

internal void ResizeBackbuffer(Backbuffer *buffer, int width, int height)
{
	printf("Resizing to %dx%d\n", width, height);
	buffer->width = width;
	buffer->height = height;

	if (buffer->pixels)
	{
		free(buffer->pixels);
	};

	buffer->pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));

	XResizeWindow(buffer->dis, buffer->win, width, height);
}

int main(void)
{
	InitializeBackbuffer(&globalBackbuffer);

	XEvent event;
	KeySym key;
	char eventMsg[X_EVENT_BUFFER_SIZE];

	while (1)
	{
		XNextEvent(globalBackbuffer.dis, &event);
		if (event.type == Expose && event.xexpose.count == 0)
		{
			// TODO(bruno): redraw
		}
		if (event.type == KeyPress &&
			XLookupString(&event.xkey, eventMsg, X_EVENT_BUFFER_SIZE, &key,
						  NULL) == 1)
		{
			if (eventMsg[0] == 'q')
				break;
		}
		if (event.type == ResizeRequest)
		{
			ResizeBackbuffer(&globalBackbuffer, event.xresizerequest.width,
							 event.xresizerequest.height);
		}
	}

	return 0;
}
