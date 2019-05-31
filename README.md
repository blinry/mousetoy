# MouseToy

Did you ever wish that attaching multiple mice to your Linux machine would give you multiple mouse cursors? Well, since X.Org 1.7 (released in 2009), you can actually do that!

Compile this program using `make`. Then, attach a few mice and run `./mousetoy 1` to set up the devices as individual cursors. Larger numbers will give you different game modes. Here's a complete list:

- `./mousetoy 0`: Reset the devices to control a single mouse cursor.
- `./mousetoy 1`: Set up the devices to control a one mouse cursor each.
- `./mousetoy 2`: Push other mouse cursors around, and bounce from the screen's edges!
- `./mousetoy 3`: Orbit around other cursors, and bounce from the screen's edges!
- `./mousetoy 4`: Tag mode! One of the cursors looks different, the others have to catch it!
