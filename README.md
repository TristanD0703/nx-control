| Supported Targets | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 | ESP32-S31 |
| ----------------- | -------- | -------- | -------- | -------- | --------- |

### Hardware Required

Any ESP board that have USB-OTG supported.

### Build and Flash

First, set your esp device target. Example:

```bash
idf.py set-target esp32s3
```

Next, find your device port in the /dev folder. On Mac you're looking for something like this:

```bash
ls /dev/cu.*

/dev/cu.usbmodemA1B2C2...
```

> [!NOTE]
> If you're not seeing any devices that start with /dev/cu. on Mac, you may need to install [some drivers](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads)

On linux you're looking for something like this:

```bash
ls /dev/tty*
```

Finally, build the project and flash it to the board, then run monitor tool to view serial output:

```bash
idf.py -p {path_from_above} flash monitor
```

(To exit the serial monitor, type `Ctrl-]`.)

This project is still in development. more to come soon.

## Build and Run Tests

The controller has the ability to test both on device (microcontroller) and on the host machine (your laptop/desktop). Device tests will come at a later date

### Host Tests

First, much like the main project, we need to build the tests using CMake.
Use the following command:

```bash
cmake -S tests -B build-test
cmake --build build-test
```

Then, run the tests:
```bash
./build-test/host/nintendo_switch_host_tests
```
