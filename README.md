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

The `target_cmock_mocks()` CMake helper creates `Mock<name>.c` and
`Mock<name>.h` under the target's `generated/<target>/mocks` build directory.
It regenerates them when the input header or `tests/cmock.yml` changes, and adds
the generated sources and include directories to the executable. Each mocked
API must have only one implementation linked into the executable, so replace
its real or fake source with the generated mock when adopting it.

Tests using a mock include its generated header and call `Mock<name>_Init()`,
`Mock<name>_Verify()`, and `Mock<name>_Destroy()` around each test case to reset
and verify expectations. The controller-state suite uses `Mocktimer.h`, generated
from `src/timer/timer.h`, with CMock lifecycle calls in `setUp()` and `tearDown()`.
It captures scheduled callbacks so tests can simulate timer expiration directly.
