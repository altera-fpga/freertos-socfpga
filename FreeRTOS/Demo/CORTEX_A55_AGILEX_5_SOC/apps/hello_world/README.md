## Hello world APP
This is a sample FreeRTOS Hello world app which will periodically print the string "hello world" to the console
(console is configured as UART 0)

**1. Building the app**

The project uses cmake to setup the build<br>
This app adds Agilex5 FreeRTOS port as dependency

copy the pfg file and sof to application source directory<br>
get the pfg file from
```bash
wget https://releases.rocketboards.org/2024.11/zephyr/agilex5/hps_zephyr/hello_world/qspi_boot/qspi_flash_image_agilex5_boot.pfg
```

```bash
cmake -B build .
cmake --build build
cd build
```

Build sd card image
```bash
make sd-image
```

Build qspi image
```bash
make qspi-image
```

**2. Setup the sd card**

Insert the sd card and find the block device path ( if the device is enumerated as /dev/mmcblk0)
Eg:
```bash
    dd of=/dev/mmcblk0 if=sd.img bs=1M status=progress
```

**3. output files**

after build the following can be found in the build directory
1. sd.img : SD card image
2. qspi_image.jic : QSPI image

