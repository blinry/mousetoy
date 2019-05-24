#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    int id1, id2;
} Context;

typedef struct PointerConfiguration {
    int master_pointers[8];
    int num_master_pointers;

    int slave_pointers[128];
    int num_slave_pointers;
} PointerConfiguration;

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

        // move pointers
        warp(context, c1.id, c1.x, c1.y);
        warp(context, c2.id, c2.x, c2.y);

        // sleep
        usleep(30000);
    }
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
    // context.id1 = id1;
    // context.id2 = id2;

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
        // printf("Device is attached to/paired with %d\n", device->attachment);
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

        int ret = XIChangeHierarchy(context->display, &add, 1);
        printf("return %d\n", ret);
    }

    pc = get_pointer_configuration(context);

    for (int i = 1; i < pc.num_slave_pointers; i++) {
        XIAttachSlaveInfo attach;
        attach.type = XIAttachSlave;
        attach.deviceid = pc.slave_pointers[i];
        attach.new_master = pc.master_pointers[i];

        int ret = XIChangeHierarchy(context->display, &attach, 1);
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

void orbits(Context context) {
    double x, y;

    query(context, context.id1, &x, &y);
    PhysicsEnt c1 = {0, 0, x, y, context.id1};

    query(context, context.id2, &x, &y);
    PhysicsEnt c2 = {0, 0, x, y, context.id2};

    printf("%f %f\n", x, y);

    loop(context, c1, c2);
}

void fling(Context context) {
    double x, y;
    query(context, context.id1, &x, &y);

    PhysicsEnt c1 = {0, 0, x, y, context.id1};

    double c = 0.1;

    while (True) {
        double newx, newy;
        query(context, context.id1, &newx, &newy);

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

    // reset_pointers(&context);
    // init_pointers(&context);

    // orbits(context);
    // fling(context);
}
