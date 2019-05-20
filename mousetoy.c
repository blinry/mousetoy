#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

typedef struct PhysicsEnt {
    double vx, vy;
    double x, y;
    int id;
} PhysicsEnt;

typedef struct Context {
    Display *display;
    Window root_window;
} Context;

void warp(Context context, int deviceid, double x, double y) {

    double src_x = 0;
    double src_y = 0;
    int src_width = 0;
    int src_height = 0;

    Window dest_w = context.root_window;
    Window src_w = None;

    int err = XIWarpPointer(context.display, deviceid, src_w, dest_w, src_x,
                            src_y, src_width, src_height, x, y);
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

void loop(Context context, PhysicsEnt c1, PhysicsEnt c2) {
    double fy, fx;
    double c = 50.0;

    while(True) {
        query(context, c1.id, &c1.x, &c1.y);
        query(context, c2.id, &c2.x, &c2.y);

        // calculate forces
        double xdiff = c1.x - c2.x;
        double ydiff = c1.y - c2.y;
        double d = sqrt(xdiff*xdiff + ydiff*ydiff);
        if(d < 5) {
            d = 5;
        }
        double f = c * (1.0/ (pow(d, 1.5) ));
        fx = xdiff/d * f;
        fy = ydiff/d * f;

        // calculate new positions
        c1.vx += -fx;
        c2.vx += fx;
        c1.vy += -fy;
        c2.vy += fy;

        printf("%f\n", c1.y);
        c1.x += c1.vx;
        c2.x += c2.vx;
        c1.y += c1.vy;
        c2.y += c2.vy;

        // move cursors
        warp(context, c1.id, c1.x, c1.y);
        warp(context, c2.id, c2.x, c2.y);

        // sleep
        usleep(30000);
    }
}

int main() {
    int first_id = 2;
    int second_id = 16;
    double x,y;

    char *env_display = getenv("DISPLAY");
    Display *display = XOpenDisplay(env_display);

    int screen = 0;

    Window root_window = RootWindow(display, screen);

    Context context;
    context.display = display;
    context.root_window = root_window;

    query(context, first_id, &x, &y);
    PhysicsEnt c1 = {0, 0, x, y, first_id};

    query(context, second_id, &x, &y);
    PhysicsEnt c2 = {0, 0, x, y, second_id};

    printf("%f %f\n", x, y);

    loop(context, c1, c2);
}
