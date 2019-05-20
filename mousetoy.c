#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Context {
    Display *display;
    Window root_window;
} Context;

void warp(Context context, int deviceid, int x, int y) {

    int src_x = 0;
    int src_y = 0;
    int src_width = 0;
    int src_height = 0;

    int dest_x = 100;
    int dest_y = 100;

    Window dest_w = context.root_window;
    Window src_w = None;

    int err = XIWarpPointer(context.display, deviceid, src_w, dest_w, src_x,
                            src_y, src_width, src_height, x, dest_y);
    XFlush(context.display);
}

void query(Context context, int deviceid, double *x, double *y) {
    Window root = 0;
    Window window = 0;

    double double_dummy = 0;
    XIButtonState button_state_dummy;
    XIModifierState modifier_state_dummy;
    XIGroupState modifier_group_dummy;

    XIQueryPointer(context.display, deviceid, context.root_window, &root,
                   &window, x, y, &double_dummy, &double_dummy,
                   &button_state_dummy, &modifier_state_dummy,
                   &modifier_group_dummy);
}

int main() {
    char *env_display = getenv("DISPLAY");
    Display *display = XOpenDisplay(env_display);

    int screen = 0;

    Window root_window = RootWindow(display, screen);

    Context context;
    context.display = display;
    context.root_window = root_window;

    double x, y;

    query(context, 14, &x, &y);
    // warp(context, 14, 100, 100);
    printf("%f %f\n", x, y);
}
