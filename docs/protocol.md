# Protocol

The ESP device accepts commands over Bluetooth Low Energy. This is accomplished by advertising the device's name, address and capabilities using the GAP protocol (a widely supported advertising protocol), and accepting read and mutation requests using GATT.

## Finding the right device

The device advertises itself under the name `NX_USB_DEVICE`. However, it does not tell you the services/characteristics it exposes over GATT. When picking from a list of devices, this name should tell it apart from the others.

Once connected, you'll see one service (UUID `a2920393bbbb6cb28e4e54945c4088`) with 2 attributes.

| Name              | UUID                              | Responsibility                            |
| ----------------- | --------------------------------- | ----------------------------------------- |
| Controller State  | 31ac59929fc58784d14e72c569b9a62c  | read and subscribe to controller updates  |
| Button            | 75c76f70c8dff385694427a8d0cce653  | send controller actions                   |

You can only write to Button, and read and subscribe from Controller State.
There is logic to prevent multiple clients from writing to the button attribute so collisions are
less likely to occur. However, many clients can read from and subscribe to the controller state.

## Device interaction

### Subscribing to updates
You can listen to updates from the controller through requesting to "notify" on the Button attribute. Once you've subscribed, each controller state change will push the exact controller state to the client.

The subscription event has the data type as follows:
```
[ [ button bitmap (2b) ][ HAT position (1b) ][ Left stick X (1b) ][ Left stick Y (1b) ][ Right stick X (1b)] [ Right stick Y (1b) ][ Unused (1b) ]
```
The button bitmap shows all of the currently pressed buttons based on using bitwise OR on all pressed buttons' numbers as shown in the next section.

"getting" the controller state characteristica value will give you the same result without subscribing you to the changes.


### Writing to "Button"
When you write to the Button characteristic, you are sending a command to the controller.
In summary, what the controller expects is a version byte, and an array of commands that dicate
what button to press and for how long, or a specific action outside the controller.

#### Data format
The data format for the command is as follows.

```
[ [Version Number (1 byte, always 0) ][ COMMAND 1 ][ COMMAND 2 ] ... ]
```

A command is made of the following

```
[ Type (1 byte) ][ Arguments (optional, variable bytes depending on type) ]
```

The type byte can be one of either CONTROLLER_RESET (0x00), RELINQUISH_WRITE (0x01), or BUTTON_COMMAND (0X02)

- The CONTROLLER_RESET command will reset all values to default settings
- The RELINQUISH_WRITE command will release the lock for writing to the controller from your connection. Other devices have the opportunity to write to the device and become the sole writer.
- The BUTTON_COMMAND sends a command to the controller. This is the only type that requires an argument

#### The BUTTON_COMMAND type
The arguments to the button command is a 14-byte struct of the following format (example)
```
| 02 00 00 00 |  | 80 00 | | 00 00 |  | 88 13 00 00 |
|-------------|  |-------| |-------|  |-------------|
 Button Type 4b  X Val 2b   Y Val 2b   Duration (millisecs) 4b
```

The first 4 bytes are for determining which kind of button you want to manipulate.
0x00 - Normal button (A, B, X, Y, ZR... etc)
0x01 - HAT/D-Pad buttons
0x02 - Left Stick
0x03 - Right Stick

The next 2 bytes are for the X value, and 2 after that are for the Y value.

The X value is used for all operations. For normal buttons and HAT buttons, it 
determines what button, and what HAT direction to press respectively. For left and right sticks
it determines the X coordinate

If you specify "Normal button", these are the valid options

```
Y                   = 0x01
B                   = 0x02
A                   = 0x04
X                   = 0x08
L Bumper            = 0x10
R Bumper            = 0x20
ZL Trigger          = 0x40
ZR Trigger          = 0x80
MINUS               = 0x100
PLUS                = 0x200
L Stick Click       = 0x400
R Stick Click       = 0x800
Home                = 0x1000
Capture             = 0x2000
```

If you specify "HAT", these are the valid options
```
HAT TOP          = 0x00
HAT TOP RIGHT    = 0x01
HAT RIGHT        = 0x02
HAT BOTTOM RIGHT = 0x03
HAT BOTTOM       = 0x04
HAT BOTTOM LEFT  = 0x05
HAT LEFT         = 0x06
HAT TOP LEFT     = 0x07
HAT CENTER       = 0x08
```

> [!WARNING]
> Only one of these options can be selected at a time. If one is already selected,
> it will be overwritten!

Left and Right stick options are self explanatory. They're the X and Y positions of the stick.
**Please note that the center of the sticks are x = 128, y = 128**, so you will need to adjust the center
based on that.

Finally, we have duration. This is how long the button or stick direction you specify will be held.
It will automatically be released when time expires.
