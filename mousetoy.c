#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_ENTITIES 7

typedef struct PhysicsEnt {
    double vx, vy;
    double x, y;
    int id;
} PhysicsEnt;

typedef struct Context {
    Display *display;
    Window root_window;
    int width;
    int height;
    int ids[MAX_ENTITIES];
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

void loop(Context context, PhysicsEnt *entities) {
    double fy, fx;
    double c = 50.0;
    PhysicsEnt c1 = entities[0];
    PhysicsEnt c2 = entities[1];

    while (True) {
        query(context, c1.id, &c1.x, &c1.y);
        query(context, c2.id, &c2.x, &c2.y);

        // calculate forces
        double xdiff = c1.x - c2.x;
        double ydiff = c1.y - c2.y;
        double d = sqrt(xdiff * xdiff + ydiff * ydiff);
        if (d < 5) {
            d = 5;
        }
        double f = c * (1.0 / (pow(d, 1.5)));
        fx = xdiff / d * f;
        fy = ydiff / d * f;

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

        if (c1.y >= context.height || c1.y <= 0) {
            c1.vy *= -1;
        }
        if (c1.x >= context.width || c1.x <= 0) {
            c1.vx *= -1;
        }
        if (c2.y >= context.height || c2.y <= 0) {
            c2.vy *= -1;
        }
        if (c2.x >= context.width || c2.x <= 0) {
            c2.vx *= -1;
        }

        // move cursors
        warp(context, c1.id, c1.x, c1.y);
        warp(context, c2.id, c2.x, c2.y);

        // sleep
        usleep(30000);
    }
    free(entities);
}

Context build_context() {
    char *env_display = getenv("DISPLAY");
    Display *display = XOpenDisplay(env_display);

    int screen_id = 0;

    Window root_window = RootWindow(display, screen_id);

    Screen *screen = ScreenOfDisplay(display, screen_id);

    // Find the master pointer devices.
    int id1, id2;

    int ndevices;
    XIDeviceInfo *devices =
        XIQueryDevice(display, XIAllMasterDevices, &ndevices);

    for (int i = 0; i < ndevices; i++) {
        XIDeviceInfo *device = &devices[i];
        printf("Device %s (id: %d) is a ", device->name, device->deviceid);

        switch (device->use) {
        case XIMasterPointer:
            printf("master pointer\n");

            if (i == 0) {
                id1 = device->deviceid;
            } else {
                id2 = device->deviceid;
            }

            break;
        case XIMasterKeyboard:
            printf("master keyboard\n");
            break;
            // case XISlavePointer:
            //    printf("slave pointer\n");
            //    break;
            // case XISlaveKeyboard:
            //    printf("slave keyboard\n");
            //    break;
            // case XIFloatingSlave:
            //    printf("floating slave\n");
            //    break;
        }

        // printf("Device is attached to/paired with %d\n", device->attachment);
    }

    XIFreeDeviceInfo(devices);

    //// Set up event listener.
    // XIEventMask eventmask;
    // unsigned char mask[1] = {0};

    // eventmask.deviceid = XIAllMasterDevices;
    // eventmask.mask_len = sizeof(mask);
    // eventmask.mask = mask;
    // XISetMask(mask, XI_ButtonPress);
    // XISetMask(mask, XI_Motion);
    //// XISetMask(mask, XI_KeyPress);

    /* select on the window */
    // XISelectEvents(display, window, &eventmask, 1);

    Context context;
    context.display = display;
    context.root_window = root_window;
    context.width = WidthOfScreen(screen);
    context.height = HeightOfScreen(screen);
    context.ids[0] = id1;
    context.ids[1] = id2;

    return context;
}

void orbits(Context context) {
    double x, y;

    query(context, context.ids[0], &x, &y);
    PhysicsEnt c1 = {0, 0, x, y, context.ids[0]};

    query(context, context.ids[1], &x, &y);
    PhysicsEnt c2 = {0, 0, x, y, context.ids[1]};

    printf("%f %f\n", x, y);


    PhysicsEnt *phys_ents = malloc(MAX_ENTITIES * sizeof(PhysicsEnt));
    /* PhysicsEnt ps[2] = {c1, c2}; */
    phys_ents[0] = c1;
    phys_ents[1] = c2;
    loop(context, phys_ents);
}

void fling(Context context) {
    double x, y;
    query(context, context.ids[0], &x, &y);

    PhysicsEnt c1 = {0, 0, x, y, context.ids[0]};

    double c = 0.1;

    while (True) {
        double newx, newy;
        query(context, context.ids[0], &newx, &newy);

        if (abs(newx - c1.x) > 1) {
            x = newx;
        } else {
            x = c1.x;
        }
        if (abs(newy - c1.y) > 1) {
            y = newy;
        } else {
            y = c1.y;
        }

        double xdiff = x - c1.x;
        double ydiff = y - c1.y;

        double limit = 0.1;

        double moved = sqrt(xdiff * xdiff + ydiff * ydiff);

        // if (abs(xdiff) > abs(c1.vx)) {
        c1.vx += c * xdiff;
        //}
        // if (abs(ydiff) > abs(c1.vy)) {
        c1.vy += c * ydiff;
        //}

        printf("%f %f\n", c1.vx, c1.vy);

        c1.x += c1.vx;
        c1.y += c1.vy;

        if (c1.y >= context.height || c1.y <= 0) {
            c1.vy *= -1;
        }
        if (c1.x >= context.width || c1.x <= 0) {
            c1.vx *= -1;
        }

        double f = 0.9;

        c1.vx *= f;
        c1.vy *= f;

        warp(context, c1.id, c1.x, c1.y);

        usleep(16000);
    }
}

int main(int argc, char **argv) {
    Context context = build_context();

    orbits(context);
    /* fling(context); */
}
