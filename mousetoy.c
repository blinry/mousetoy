#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/extensions/XInput.h>

#define MAX_ENTITIES 7

typedef enum Mode { MODE_ORBIT, MODE_PUSH } Mode;

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
            printf("master %d\n", device->deviceid);
            break;
        case XISlavePointer:
        case XIFloatingSlave:
            pc.slave_pointers[pc.num_slave_pointers] = device->deviceid;
            pc.num_slave_pointers++;
            printf("slave %d\n", device->deviceid);
            break;
        }
    }

    XIFreeDeviceInfo(devices);

    return pc;
}

void init_pointers(Context *context) {
    PointerConfiguration pc = get_pointer_configuration(context);

    for (int i = 1; i < pc.num_slave_pointers; i++) {
        printf("creating new master\n");

        XIAddMasterInfo add;
        add.type = XIAddMaster;
        add.name = "mousetoy";
        add.send_core = True;
        add.enable = True;

        int ret = XIChangeHierarchy(context->display,
                                    (XIAnyHierarchyChangeInfo *)&add, 1);
        printf("return %d\n", ret);
    }

    pc = get_pointer_configuration(context);

    for (int i = 1; i < pc.num_slave_pointers; i++) {
        XIAttachSlaveInfo attach;
        attach.type = XIAttachSlave;
        attach.deviceid = pc.slave_pointers[i];
        attach.new_master = pc.master_pointers[i];

        int ret = XIChangeHierarchy(context->display,
                                    (XIAnyHierarchyChangeInfo *)&attach, 1);
        printf("return %d\n", ret);
    }
}

void reset_pointers(Context *context) {
    PointerConfiguration pc = get_pointer_configuration(context);
    for (int i = 1; i < pc.num_master_pointers; i++) {
        printf("deleting %d\n", pc.master_pointers[i]);
        XIRemoveMasterInfo remove;
        remove.type = XIRemoveMaster;
        remove.deviceid = pc.master_pointers[i];
        remove.return_mode = XIAttachToMaster;
        remove.return_pointer = pc.master_pointers[0];
        remove.return_keyboard = 3; // FIXME: Our new master devices *should*
                                    // not have any keyboard slaves...

        int ret = XIChangeHierarchy(context->display,
                                    (XIAnyHierarchyChangeInfo *)&remove, 1);
        printf("return %d\n", ret);
    }
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
}

void setup_pointers(Context ctx) {
    XIDeviceInfo *info;
    int ndevices;

    info = XIQueryDevice(ctx.display, XIAllDevices, &ndevices);
    for(int i=0; i<ndevices; i++) {
        printf("%i: ");

        // using code from xlib
        switch (info->use) {
        case IsXPointer:
            printf("XPointer");
            break;
        case IsXKeyboard:
            printf("XKeyboard");
            break;
        case IsXExtensionDevice:
            printf("XExtensionDevice");
            break;
        case IsXExtensionKeyboard:
            printf("XExtensionKeyboard");
            break;
        case IsXExtensionPointer:
            printf("XExtensionPointer");
            break;
        default:
            printf("Unknown class");
            break;
        }
        printf("\n");
        info += 1;
    }
}

int main(int argc, char **argv) {
    Mode mode = MODE_PUSH;
    if (argc > 1) {
        mode = atoi(argv[1]);
    }


    Context context = build_context();
    context.mode = mode;

    setup_pointers(context);

    register_pointers(&context);
    loop(&context);
}
