#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>

void warp(int deviceid, int x, int y) {
    int src_x = 0;
    int src_y = 0;
    int src_width = 0;
    int src_height = 0;

    char *env_display = getenv("DISPLAY");

    Display *display = XOpenDisplay(env_display);

    int screen = 0;

    Window screen_root = RootWindow(display, screen);

    Window dest_w = screen_root;
    Window src_w = None;

    int dest_x = 100;
    int dest_y = 100;

    int err = XIWarpPointer(display, deviceid, src_w, dest_w, src_x, src_y,
                            src_width, src_height, dest_x, dest_y);
    XFlush(display);
}

int main() {
    int deviceid = 14;

    warp(14, 100, 100);
}
