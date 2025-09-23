#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <cstdint>
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
	int pitch;
	XImage *ximage;
};

global_variable Backbuffer globalBackbuffer;

global_variable int xOffset = 0;
global_variable int yOffset = 0;

global_variable bool isRunning = true;

void InitializeX11(Backbuffer *backbuffer)
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

	backbuffer->width = 1280;
	backbuffer->height = 720;
	backbuffer->pitch = backbuffer->width * sizeof(uint32_t);

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
	XSelectInput(
		backbuffer->dis, backbuffer->win,
		ExposureMask | ButtonPressMask | KeyPressMask |
			StructureNotifyMask); // TODO(bruno): figure out why we couldn't use
								  // `ResizeRedirectMask` and had to switch to
								  // `StructureNotifyMask`

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
}

void InitializeBackbuffer(Backbuffer *backbuffer)
{
	void *imagedata =
		malloc(backbuffer->width * backbuffer->height * sizeof(uint32_t));

	backbuffer->ximage = XCreateImage(
		backbuffer->dis, DefaultVisual(backbuffer->dis, backbuffer->screen), 24,
		ZPixmap, 0, (char *)imagedata, backbuffer->width, backbuffer->height,
		32, backbuffer->pitch);
};

internal void ResizeBackbuffer(Backbuffer *buffer, int width, int height)
{
	buffer->width = width;
	buffer->height = height;
	buffer->pitch = buffer->width * sizeof(uint32_t);

	if (buffer->ximage)
	{
		XDestroyImage(buffer->ximage);
	}

	InitializeBackbuffer(buffer);
}

int main(void)
{
	InitializeX11(&globalBackbuffer);
	InitializeBackbuffer(&globalBackbuffer);

	XEvent event;
	KeySym key;
	char eventMsg[X_EVENT_BUFFER_SIZE];

	while (isRunning)
	{
		// Process all pending events
		while (XPending(globalBackbuffer.dis))
		{
			XNextEvent(globalBackbuffer.dis, &event);
			if (event.type == KeyPress &&
				XLookupString(&event.xkey, eventMsg, X_EVENT_BUFFER_SIZE, &key,
							  NULL) == 1)
			{
				if (eventMsg[0] == 'q')
					isRunning = false;
			}
			if (event.type == ConfigureNotify)
			{
				ResizeBackbuffer(&globalBackbuffer, event.xconfigure.width,
								 event.xconfigure.height);
			}
		}

		// Render every frame
		GameBackBuffer gamebackbuffer;
		gamebackbuffer.width = globalBackbuffer.width;
		gamebackbuffer.height = globalBackbuffer.height;
		gamebackbuffer.pitch = globalBackbuffer.pitch;
		gamebackbuffer.memory = (uint32_t *)globalBackbuffer.ximage->data;
		GameUpdateAndRender(&gamebackbuffer, xOffset, yOffset);

		XPutImage(globalBackbuffer.dis, globalBackbuffer.win,
				  globalBackbuffer.gc, globalBackbuffer.ximage, 0, 0, 0, 0,
				  globalBackbuffer.width, globalBackbuffer.height);
		XFlush(globalBackbuffer.dis);

		xOffset++;
		yOffset++;
	}

	return 0;
}
