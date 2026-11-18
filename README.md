# NX Control
This firmware allows you to programmatically control the Nintendo Switch over Bluetooth Low Energy using a simple microcontroller that costs <$10 USD.
All you need is an ESP32-S3, a USB A-C cable, and a computer :)

## Why does this project exist?
A lot of other solutions require a lot of hardware tinkering and extra components that are hard to get your head around if you don't know what you're doing.
Additionally, these solutions require your laptop or other computer to be physically connected to the microcontroller, so you cannot move your computer while you
are executing scripts.

Some other projects attempt to solve this by allowing your computer to act as a bluetooth controller directly to the switch. However, Nintendo likes to change the
Bluetooth specification a lot, so it's hard to keep up with those changes. One thing that never changes is the USB protocol. Which is why I decided to relay commands
over USB from Bluetooth Low Energy.

Finally, I could not find a project that, even if the device worked, did not have a good interface for sending commands in a scriptable way (press x button for y amount of time).
Thus, NX Control was born!

## How to use
You must have one of these microcontrollers. [This is the one that I use for testing](https://www.amazon.com/DORHEA-Development-Bluetooth-ESP32-S3-DevKit-ESP32-S3-WROOM/dp/B0GBT212KM/ref=sr_1_3?crid=2LWAKUASWBTQ&dib=eyJ2IjoiMSJ9.FVAMjaTuvap9YBbnA1E6kFL0pzvlUb2sOYr2tgQ2chpdLtE5a_fwL0gAI52Hwf5mY9c9BzLcWwBdb4jx3h557ZfIx9e6c-KNspxKWH4-PuGUEoI97CM4lnuUP8kpwMe_vSCiZRw0FzC7q8bwBX8ayusRxR1crlnln58tl9rfEIMgpNfruk1jh7O1lo5y-Pv9sEm5I3VCgZAV0CN61H0roZIJDysasnT032Em49INvzU.6RYBv0BjslBpSiCPQ7XL3r-f7cYKqjeuwZBazOpNfvM&dib_tag=se&keywords=esp32s3&qid=1794972280&sprefix=esp32s3%2Caps%2C147&sr=8-3&th=1). The others I have not verified, but are supported according to the spec sheet/documentation.

| Supported Targets | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 | ESP32-S31 |
| ----------------- | -------- | -------- | -------- | -------- | --------- |


### Flash a Release (ESP32-S3)

Download `nx-control-esp32s3.bin` from the **Assets** section of a
[GitHub Release](https://github.com/TristanD0703/nintendo-switch-usb-controller/releases).
This image includes the bootloader, partition table, and application, so you can
flash it directly without installing ESP-IDF or building the project.

Connect the board to your computer using the **COM** port if it has two USB ports.
Install [esptool](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/)
and flash the downloaded image:

```bash
python -m pip install esptool==5.4.0
python -m esptool --chip esp32s3 --port YOUR_PORT write-flash 0x0 nx-control-esp32s3.bin
```

Replace `YOUR_PORT` with your board's serial port, such as `/dev/ttyUSB0` on Linux,
`/dev/cu.usbserial-...` on macOS, or `COM3` on Windows. If you use a graphical
flasher, select **ESP32-S3** and set the image's flash address to **`0x0`**.

After flashing, connect the board to the Switch using its **USB** port instead of
the **COM** port. The firmware is then ready for Bluetooth commands described in
the [protocol docs](./docs/protocol.md).

### Build and Flash from Source

To get the code on the device, you'll need to build the code and flash the firmware onto the device.
First you must install the prerequisite tooling from Espressif in order to interact with the device. Follow [this](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#installation) guide to install esp-idf

Once you have that installed, you can get started.

> [!NOTE]
> Depending on your installation, you might have to activate your tooling with `get_idf` before running any idf commands

First open a terminal, clone the Git repository, `cd ./nx-control`, and set your esp target. For example:

```bash
idf.py set-target esp32s3
```

Next, plug in your device. If you have 2 USB-C ports, flip the device over and make sure you plug in the one that says `COM`

Once its plugged in, find your device port in the /dev folder. On Mac you're looking for something like this:

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

You should see a flurry of output. This is normal and means your device booted up!
(To exit the serial monitor, type `Ctrl-]`.)

If you made it this far without any issues, you successfully flashed the firmware. Yay!!
Now, you can unplug the device from your computer, and plug it into the switch. Again, if you have 2 USB ports, ensure that the USB cable is plugged into the
__USB port__ instead of the __COM port__. USB does not work over the COM port!

The device will not do anything, but that's ok! It is awaiting your input over Bluetooth for controller commands. For more information on how to send and execute commands, see the [protocol docs](./docs/protocol.md)

### Publishing a Release

The CI workflow uploads a `firmware-esp32s3` bundle containing the firmware build
outputs. To publish a release, tag a commit that has been pushed to `main`:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The release workflow waits for successful CI for that exact commit, downloads
its firmware bundle, and merges the existing binaries into
`nx-control-esp32s3.bin`. It then publishes a GitHub Release with the `.bin` as a
downloadable asset. Re-running the release workflow updates the existing asset.

If the CI artifact is missing or has expired, re-run CI for the tagged commit,
then re-run the release workflow. The CI run must include the firmware-bundle
upload step.

## Build and Run Tests

The controller has the ability to test both on device (microcontroller) and on the host machine (your laptop/desktop). Device tests will come at a later date

### Host Tests

Host tests require a C compiler, CMake 3.22 or newer, and Ruby 3.0 or newer.
CMake downloads pinned versions of Unity and CMock during configuration. Ruby
runs CMock's mock generator during the build; it is not needed to run the test executable.

First, much like the main project, we need to build the tests using CMake.
Use the following command:

```bash
cmake -S tests -B build-test
cmake --build build-test
```

Then, run the tests:
```bash
ctest --test-dir build-test
```

#### Generating mocks

Add dependency header paths to `HOST_MOCK_HEADERS` in
`tests/host/CMakeLists.txt` to generate and compile their mocks into the existing
host-test executable. Paths are relative to `tests/host`, for example
`../../src/storage/storage.h` for a future storage module.

Each header file being mocked with CMock will generate `Mock<name>.c` and
`Mock<name>.h`. Each test using that mock is required to call `Mock<name>_Init()`
before each test case, and `Mock<name>_Verify()`, and `Mock<name>_Destroy()` after
each test case for the mocks to behave properly.
