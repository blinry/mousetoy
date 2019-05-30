#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xcursor/Xcursor.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ENTITIES 7

typedef enum Mode { MODE_RESET, MODE_SETUP, MODE_PUSH, MODE_ORBIT, MODE_TAG } Mode;

typedef struct PhysicsEnt {
    double vx, vy;
    double x, y;
    int id;
} PhysicsEnt;

typedef struct Context {
    Mode mode;
    Display *display;
    Window root_window;
    int width;
    int height;
    int ids[MAX_ENTITIES];
    int num_ents;
    int it_id; // only valid if mode == MODE_TAG
    unsigned long int frame;
} Context;

typedef struct PointerConfiguration {
    int master_pointers[8];
    int num_master_pointers;

    int slave_pointers[128];
    int num_slave_pointers;
} PointerConfiguration;

void warp(Context *context, int deviceid, double x, double y) {
    double src_x = 0;
    double src_y = 0;
    int src_width = 0;
    int src_height = 0;

    Window dest_w = context->root_window;
    Window src_w = None;

    int err = XIWarpPointer(context->display, deviceid, src_w, dest_w, src_x,
                            src_y, src_width, src_height, x, y);
    XFlush(context->display);
}

void query(Context *context, int deviceid, double *x, double *y) {
    Window root = 0;
    Window window = 0;

    double double_dummy = 0;
    XIButtonState button_state_dummy;
    XIModifierState modifier_state_dummy;
    XIGroupState modifier_group_dummy;

    XIQueryPointer(context->display, deviceid, context->root_window, &root,
                   &window, x, y, &double_dummy, &double_dummy,
                   &button_state_dummy, &modifier_state_dummy,
                   &modifier_group_dummy);
    if(window) {
        /* XFree(children); */
    }
}

int change_cursor(Context *context, char *img_name, int deviceid) {
    unsigned int shape;
    shape = XcursorLibraryShape(img_name);

    if (shape < 0) {
        puts("failed to get shape\n");
    }

    int size = XcursorGetDefaultSize(context->display);
    if (size == 0) {
        puts("fialed to get cursor size");
    }

    char *theme = XcursorGetTheme(context->display);
    if (theme == NULL) {
        puts("can't get cursor theme");
    }

    XcursorImage *image = XcursorShapeLoadImage(shape, theme, size);
    if (image == NULL) {
        puts("can't get cursor image\n");
    }

    Cursor cursor = XcursorImageLoadCursor(context->display, image);

    /* Window root_return; */
    /* Window parent_return; */
    /* Window *children; */
    /* unsigned int n_children; */
    /* XQueryTree(context->display, context->root_window, &root_return, &parent_return, &children, &n_children); */
    /* for(int i=0; i<n_children; i++) { */
    /*     puts("window loop"); */
    /*     int ret = XUndefineCursor(context->display, children[i]); */
    /*     printf(" ret = %d ", ret); */
    /*     /1* XIDefineCursor(context->display, deviceid, children[i], cursor); *1/ */
    /* } */
    XIDefineCursor(context->display, deviceid, context->root_window, cursor);
    XFlush(context->display);
    return 0;
}

void orbit_loop(Context *context, PhysicsEnt *entities) {
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
        for (int i = 0; i < context->num_ents; ++i) {
            query(context, context->ids[i], &xs[i], &ys[i]);
        }

        // calculate forces

        for (int i = 0; i < context->num_ents; ++i) {
            /* double xdiff = c1.x - c2.x; */
            /* double ydiff = c1.y - c2.y; */

            for (int j = 0; j < context->num_ents; j++) {
                if (i == j) {
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
                printf("i,j: %d,%d\nfx: %f\nfy: %f\n", i, j, entities[i].x,
                       entities[i].y);
            }
        }

        // TODO: missing foce calculation, this need to move into the above
        // inner loop
        for (int i = 0; i < context->num_ents; ++i) {
            // calculate new positions
            entities[i].vx += -fxs[i];
            entities[i].vy += -fys[i];

            entities[i].x += entities[i].vx;
            entities[i].y += entities[i].vy;
        }

        for (int i = 0; i < context->num_ents; ++i) {
            if (entities[i].y >= context->height || entities[i].y <= 0) {
                entities[i].vy *= -1;
            }
            if (entities[i].x >= context->width || entities[i].x <= 0) {
                entities[i].vx *= -1;
            }
        }

        // move cursors
        for (int i = 0; i < context->num_ents; ++i) {
            warp(context, context->ids[i], entities[i].x, entities[i].y);
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
    context.frame = 0;

    return context;
}

PointerConfiguration get_pointer_configuration(Context *context) {
    PointerConfiguration pc = {0};

    int ndevices;
    XIDeviceInfo *devices =
        XIQueryDevice(context->display, XIAllDevices, &ndevices);

    for (int i = 0; i < ndevices; i++) {
        XIDeviceInfo *device = &devices[i];
        switch (device->use) {
        case XIMasterPointer:
            pc.master_pointers[pc.num_master_pointers] = device->deviceid;
            pc.num_master_pointers++;
            break;
        case XISlavePointer:
        case XIFloatingSlave:
            if (!strstr(device->name, "XTEST")) {
                pc.slave_pointers[pc.num_slave_pointers] = device->deviceid;
                pc.num_slave_pointers++;
            }
            break;
        }
    }

    XIFreeDeviceInfo(devices);

    return pc;
}

void init_pointers(Context *context) {
    PointerConfiguration pc = get_pointer_configuration(context);

    for (int i = 1; i < pc.num_slave_pointers; i++) {

        XIAddMasterInfo add;
        add.type = XIAddMaster;
        add.name = "mousetoy";
        add.send_core = True;
        add.enable = True;

        int ret = XIChangeHierarchy(context->display,
                                    (XIAnyHierarchyChangeInfo *)&add, 1);
    }

    pc = get_pointer_configuration(context);

    for (int i = 1; i < pc.num_slave_pointers; i++) {
        XIAttachSlaveInfo attach;
        attach.type = XIAttachSlave;
        attach.deviceid = pc.slave_pointers[i];
        attach.new_master = pc.master_pointers[i];

        int ret = XIChangeHierarchy(context->display,
                                    (XIAnyHierarchyChangeInfo *)&attach, 1);
    }
    XFlush(context->display);
}

void reset_pointers(Context *context) {
    PointerConfiguration pc = get_pointer_configuration(context);
    for (int i = 1; i < pc.num_master_pointers; i++) {
        XIRemoveMasterInfo remove;
        remove.type = XIRemoveMaster;
        remove.deviceid = pc.master_pointers[i];
        remove.return_mode = XIAttachToMaster;
        remove.return_pointer = pc.master_pointers[0];
        remove.return_keyboard =
            3; // FIXME: Make sure this device id actually exists.

        int ret = XIChangeHierarchy(context->display,
                                    (XIAnyHierarchyChangeInfo *)&remove, 1);
    }
    XFlush(context->display);
}

void orbits(Context *context) {
    double x, y;
    PhysicsEnt *phys_ents = malloc(MAX_ENTITIES * sizeof(PhysicsEnt));

    for (int i = 0; i < context->num_ents; ++i) {
        query(context, context->ids[i], &x, &y);
        PhysicsEnt e = {0, 0, x, y, context->ids[i]};
        phys_ents[i] = e;
        printf("%f %f\n", x, y);
    }

    orbit_loop(context, phys_ents);
}

void loop(Context *context) {
    double x, y;
    PhysicsEnt *phys_ents = malloc(MAX_ENTITIES * sizeof(PhysicsEnt));

    for (int i = 0; i < context->num_ents; ++i) {
        query(context, context->ids[i], &x, &y);
        PhysicsEnt e = {0, 0, x, y, context->ids[i]};
        phys_ents[i] = e;
    }

    double c = 0.05;

    if (context->mode == MODE_ORBIT) {
        c = 0.02;
    }

    while (True) {
        context->frame += 1;
        for (int i = 0; i < context->num_ents; ++i) {
            double newx, newy;
            query(context, context->ids[i], &newx, &newy);

            if (abs(newx - phys_ents[i].x) > 1) {
                x = newx;
            } else {
                x = phys_ents[i].x;
            }
            if (abs(newy - phys_ents[i].y) > 1) {
                y = newy;
            } else {
                y = phys_ents[i].y;
            }

            double xdiff = x - phys_ents[i].x;
            double ydiff = y - phys_ents[i].y;

            double limit = 0.1;

            double moved = sqrt(xdiff * xdiff + ydiff * ydiff);

            phys_ents[i].vx += c * xdiff;
            phys_ents[i].vy += c * ydiff;

            phys_ents[i].x += phys_ents[i].vx;
            phys_ents[i].y += phys_ents[i].vy;

            if (phys_ents[i].y >= context->height || phys_ents[i].y <= 0) {
                phys_ents[i].vy *= -1;
            }
            if (phys_ents[i].x >= context->width || phys_ents[i].x <= 0) {
                phys_ents[i].vx *= -1;
            }

            for (int j = 0; j < context->num_ents; ++j) {
                if (i == j) {
                    continue;
                }

                double xdiff = phys_ents[i].x - phys_ents[j].x;
                double ydiff = phys_ents[i].y - phys_ents[j].y;
                double d = sqrt(xdiff * xdiff + ydiff * ydiff);

                if (context->mode == MODE_ORBIT) {
                    double c2 = 0.1;

                    phys_ents[i].vx -= c2 * xdiff / d;
                    phys_ents[i].vy -= c2 * ydiff / d;

                    phys_ents[j].vx += c2 * xdiff / d;
                    phys_ents[j].vy += c2 * ydiff / d;
                } else if (context->mode == MODE_PUSH) {
                    double c2 = 0.01;

                    if (d < 100) {
                        phys_ents[i].vx += c2 * xdiff;
                        phys_ents[i].vy += c2 * ydiff;

                        phys_ents[j].vx -= c2 * xdiff;
                        phys_ents[j].vy -= c2 * ydiff;
                    }
                } else if (context->mode == MODE_TAG) {
                    if (context->ids[i] == context->it_id) {
                        if (d < 50 && context->frame > 62) {
                            context->frame = 0;
                            puts("switch");
                            context->it_id = context->ids[j];
                            change_cursor(context, "crosshair", context->ids[j]);
                            change_cursor(context, "right_ptr", context->ids[i]);
                        }
                    }
                }
            }

            double f = 0.9;
            if (context->mode == MODE_ORBIT) {
                f = 0.99;
            }

            phys_ents[i].vx *= f;
            phys_ents[i].vy *= f;

            warp(context, context->ids[i], phys_ents[i].x, phys_ents[i].y);
        }

        usleep(16000);
    }
}

void register_pointers(Context *context) {
    PointerConfiguration pc = get_pointer_configuration(context);

    context->num_ents = pc.num_master_pointers;
    for (int i = 0; i < pc.num_master_pointers; i++) {
        context->ids[i] = pc.master_pointers[i];
    }
    context->it_id = context->ids[0];
}

int main(int argc, char **argv) {
    Mode mode = MODE_PUSH;
    if (argc > 1) {
        mode = atoi(argv[1]);
    }

    Context context = build_context();
    context.mode = mode;

    if (mode == MODE_RESET) {
        reset_pointers(&context);
        exit(0);
    }

    if (mode == MODE_SETUP) {
        init_pointers(&context);
        exit(0);
    }

    register_pointers(&context);

    loop(&context);
}
