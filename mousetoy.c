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
    int num_ents;
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
    /* double fy, fx; */
    double c = 50.0;
    PhysicsEnt c1 = entities[0];
    PhysicsEnt c2 = entities[1];
    double xs[MAX_ENTITIES];
    double ys[MAX_ENTITIES];
    double fxs[MAX_ENTITIES] = {0};
    double fys[MAX_ENTITIES] = {0};

    while (True) {
        /* query(context, c1.id, &c1.x, &c1.y); */
        /* query(context, c2.id, &c2.x, &c2.y); */
        for(int i=0; i<context.num_ents; ++i) {
            query(context, context.ids[i], &xs[i], &ys[i]);
        }

        // calculate forces

        for(int i=0; i<context.num_ents; ++i) {
            /* double xdiff = c1.x - c2.x; */
            /* double ydiff = c1.y - c2.y; */

            for(int j=0; j<context.num_ents; j++) {
                if(i == j) {
                    continue;
                }

                double xdiff = xs[i] - xs[j];
                double ydiff = ys[i] - ys[j];
                double d = sqrt(xdiff * xdiff + ydiff * ydiff);
                if (d < 5) {
                    d = 5;
                }
                double f = c * (1.0 / (pow(d, 1.5)));
                fxs[i] += xdiff / d * f;
                fys[i] += ydiff / d * f;
                printf("i,j: %d,%d\nfx: %f\nfy: %f\n", i, j, entities[i].x, entities[i].y);
            }
        }

        // TODO: missing foce calculation, this need to move into the above inner loop
        for(int i=0; i<context.num_ents; ++i) {
            // calculate new positions
            entities[i].vx += -fxs[i];
            entities[i].vy += -fys[i];

            entities[i].x += entities[i].vx;
            entities[i].y += entities[i].vy;
        }

        for(int i=0; i<context.num_ents; ++i) {
            if (entities[i].y >= context.height || entities[i].y <= 0) {
                entities[i].vy *= -1;
            }
            if (entities[i].x >= context.width || entities[i].x <= 0) {
                entities[i].vx *= -1;
            }
        }

        // move cursors

        for(int i=0; i<context.num_ents; ++i) {
            warp(context, context.ids[i], entities[i].x, entities[i].y);
        }

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
    context.num_ents = 2;

    return context;
}

void orbits(Context context) {
    double x, y;
    PhysicsEnt *phys_ents = malloc(MAX_ENTITIES * sizeof(PhysicsEnt));

    for(int i=0; i<context.num_ents; ++i) {
        query(context, context.ids[i], &x, &y);
        PhysicsEnt e = {0, 0, x, y, context.ids[i]};
        phys_ents[i] = e;
        printf("%f %f\n", x, y);
    }

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
