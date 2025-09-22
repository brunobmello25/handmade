#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define internal static
#define global_variable static
#define local_persist static

#define X_EVENT_BUFFER_SIZE 255

#include "handmade.cpp"

Display *dis;
int screen;
Window win;
GC gc;

void init_x()
{
	/* get the colors black and white (see section for details) */
	unsigned long black, white;

	/* use the information from the environment variable DISPLAY
	   to create the X connection:
	*/
	dis = XOpenDisplay((char *)0);
	screen = DefaultScreen(dis);
	black = BlackPixel(dis, screen),	 /* get color black */
		white = WhitePixel(dis, screen); /* get color white */

	/* once the display is initialized, create the window.
	   This window will be have be 200 pixels across and 300 down.
	   It will have the foreground white and background black
	*/
	win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0, 200, 300, 5,
							  white, black);

	/* here is where some properties of the window can be set.
	   The third and fourth items indicate the name which appears
	   at the top of the window and the name of the minimized window
	   respectively.
	*/
	XSetStandardProperties(dis, win, "Handmade Hero", "HI!", None, NULL, 0,
						   NULL);

	/* this routine determines which types of input are allowed in
	   the input.  see the appropriate section for details...
	*/
	XSelectInput(dis, win, ExposureMask | ButtonPressMask | KeyPressMask);

	/* create the Graphics Context */
	gc = XCreateGC(dis, win, 0, 0);

	/* here is another routine to set the foreground and background
	   colors _currently_ in use in the window.
	*/
	XSetBackground(dis, gc, white);
	XSetForeground(dis, gc, black);

	/* clear the window and bring it on top of the other windows */
	XClearWindow(dis, win);
	XMapRaised(dis, win);
};

int main(void)
{
	init_x();

	XEvent event;
	KeySym key;
	char eventMsg[X_EVENT_BUFFER_SIZE];

	while (1)
	{
		XNextEvent(dis, &event);
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
	}

	return 0;
}
