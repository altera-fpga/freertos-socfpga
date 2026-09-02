## Freertos OS sample APPs

This folder contains some basic freertos sample apps and tests.

1. Basic CLI application
    * A simple CLI app which exposes basic features of various drivers available in the platform
2. Ethernet sample application
    * Sample APPs using the ethernet stack
    * one of the sample can be selected in file **main_freertosplus_basic/main_freertosplus_basic.h**<br>
    Following are the available samples, *only one can be enabled at a time*
        * DEMO_PING<br>
        The device will ping an external machine (define the external device IP in the same header file)
        * DEMO_TCP<br>
        A sample TCP server, use port 9640 (can be changed in the header file), server will acknowledge the messages received
        * DEMO_UDP<br>
        A sample UDP app, use port 8897 (can be changed in the header file)
        * DEMO_ECHOTCP<br>
        A TCP server which will echo back any message sent to it
        * DEMO_IPERF<br>
        Will run an Iperf server instance
        * Default app is **DEMO_ECHOTCP**
3. OS tests
    * This test basic kernel functionalities
    * Enable/disable the tests in FreeRTOSConfig.h file in directory<br>
    **FreeRTOS/Demo/SOCFPGA/FreeRTOSConfig.h**
    * By default all tests are enabled.
4. FreeRTOS blinky app
    * A simple blinky app which queues and dequeues a message in the queue<br>
    the dequeued message is printed in the console
5. Heterogeneous SMP Multi Block Test
    *   This HSMP test application runs multiple modules in parallel on cores that can be enabled/disabled from **FreeRTOS/Demo/SOCFPGA/FreeRTOSConfigSMP.h**.
    *   These modules can also be enabled/disabled to run in parallel by enabling the respective macros inside **heterogeneous_multi_block_test/heterogeneous_multi_block_test_config.h**
    *   The modules can also be pinned to specific cores by enabling the *pining* feature in **heterogeneous_multi_block_test/heterogeneous_multi_block_test_config.h**.
    *   The modules are as follows:
        * **SDMMC:**
        Performs fatfs operations, opens files, writes data, reads back data, compares data and closes file till stop is requested.
        * **SDM Mailbox:**
        Opens a mailbox client, continuously requests temperature and voltage reading from SDM till stop is requested.
        * **QSPI:**
        Erases and writes sectors with data. Then application reads back from sectors till stop is requested.
        * **Ethernet:**
        Sets up a static Ipv4 address and runs an echo server, receives and echoes 256 byts.
        * **RTOS:**
        Verifies working of FreeRTOS constructs like queues, mutex, stream buffer, message buffer and event group.

**Building the APPs**

The project uses CMake/Ninja to setup the build. These apps add Agilex5 FreeRTOS port as dependency.

To build **HSMP test application**, navigate to the application folder and execute the below commands:

   * Using CMake:

```bash
cmake -B build . -DHSMP=ON -DCMAKE_BUILD_TYPE=Release -DSOC=AGILEX5 -DCORE=A55 -DSOF_PATH=<path to .sof file> -DPFG_QSPI=<.pfg for qspi>
cmake --build build
```

   * Using Ninja:

```bash
cmake -B build -G Ninja . -DHSMP=ON -DCMAKE_BUILD_TYPE=Release -DSOC=AGILEX5 -DCORE=A55 -DSOF_PATH=<path to .sof file> -DPFG_QSPI=<.pfg for qspi>
ninja -C build qspi-image
```

To build **other** applications, navigate to the application folder and execute the below commands:-

   * Using CMake:

```bash
cmake -B build . -DCMAKE_BUILD_TYPE=Release -DSOC=AGILEX5 -DCORE=A55 -DSOF_PATH=<path to .sof file> -DPFG_SDMMC=<.pfg for sd/mmc> -DPFG_QSPI=<.pfg for qspi>
cmake --build build
```

   * Using Ninja:

```bash
cmake -B build -G Ninja . -DCMAKE_BUILD_TYPE=Release -DSOC=AGILEX5 -DCORE=A55 -DSOF_PATH=<path to .sof file> -DPFG_SDMMC=<.pfg for sd/mmc> -DPFG_QSPI=<.pfg for qspi>
ninja -C build qspi-image
```