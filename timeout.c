/*
 * timeout.c
 *
 * how to set a timeout and/or wait for some file descriptor within an X11
 * event loop
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <X11/Xlib.h>

/*
 * wait for the next X11 event, stdin data or timeout (in milliseconds)
 */
int nextevent(Display *dsp, Window win, int timeout, XEvent *evt) {
	fd_set fds;
	int max, ret;
	struct timeval tv;
	int end, current;

	/* ending time */
	gettimeofday(&tv, NULL);
	end = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;

	/* some event may already be in queue while the x11 socket is inactive;
	 * this is why the queue is checked before the select() */
	while (! XCheckWindowEvent(dsp, win, KeyPressMask, evt)) {

		FD_ZERO(&fds);

		/* wait for the x11 socket */
		FD_SET(ConnectionNumber(dsp), &fds);
		max = ConnectionNumber(dsp);

		/* wait for standard input */
		FD_SET(STDIN_FILENO, &fds);
		max = max < STDIN_FILENO ? STDIN_FILENO : max;

		/* how long to wait */
		gettimeofday(&tv, NULL);
		current = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		if (current >= end)
			current = end;
		tv.tv_sec = (end - current) / 1000;
		tv.tv_usec = ((end - current) % 1000 ) * 1000;

		/* select */
		ret = select(max + 1, &fds, NULL, NULL, &tv);
		if (ret == -1)
			continue;	/* error, maybe a signal */

		if (FD_ISSET(ConnectionNumber(dsp), &fds)) 
			continue;	/* X11 activity, maybe an event */

		if (FD_ISSET(STDIN_FILENO, &fds))
			return 1;	/* data from stdin */

		return 0;		/* timeout */
	}
	return 2;			/* X11 event */
}

/*
 * main
 */
int main() {
	Display *dsp;
	Window win;
	Screen *scr;
	int stayinloop, ret;
	XEvent evt;
	char buf[1000];
	int len, key;

				/* open display, create window */

	dsp = XOpenDisplay(NULL);
	if (dsp == NULL) {
		printf("cannot open display\n");
		exit(EXIT_FAILURE);
	}
	scr = DefaultScreenOfDisplay(dsp);

	win = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp),
		200, 200, 400, 300, 1,
		BlackPixelOfScreen(scr), WhitePixelOfScreen(scr));
	XSelectInput(dsp, win, KeyPressMask);
	XStoreName(dsp, win, "timeout");

	XMapWindow(dsp, win);

				/* event loop */

	for (stayinloop = 1; stayinloop; ) {
		ret = nextevent(dsp, win, 2000, &evt);
		switch (ret) {
		case 0:
			printf("timeout\n");
			break;
		case 1:
			len = read(STDIN_FILENO, buf, 900);
			buf[len] = '\0';
			printf("stdin: %s\n", buf);
			break;
		case 2:
			printf("x11 event");
			if (evt.type != KeyPress) {
				printf("\n");
				break;
			}
			key = XLookupKeysym(&evt.xkey, 0);
			printf(" ---> key %c\n", key);
			if (key == 'q')
				stayinloop = 0;
			break;
		}
		fflush(stdout);
	}

	XCloseDisplay(dsp);

	return EXIT_SUCCESS;
}
