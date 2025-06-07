## Getting started

This repository contains FreeRTOS port for Agilex5 HPS. The FreeRTOS code is added
as a submodule inside this repository.

For FreeRTOS kernel feature information refer to the
[Developer Documentation](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/00-Developer-docs),
and [API Reference](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/00-TaskHandle).

## Getting toolchain

Toolchain can be downloaded from ARM developer website
[Here](https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf.tar.xz) is the link for downloading the toolchain

### Installing the toolchain

```bash
AARCH64_TOOLCHAINPATH=<toolchain path>
mkdir -p $AARCH64_TOOLCHAINPATH
cd $AARCH64_TOOLCHAINPATH
cp <download folder>/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf.tar.xz .
tar -xf arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf.tar.xz
rm arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf.tar.xz
export PATH=$AARCH64_TOOLCHAINPATH/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf/bin/:$PATH
```
## Getting Quartus
Follow the steps in the official website.<br>
Download and install [Quartus](https://www.intel.com/content/www/us/en/software-kit/851652/intel-quartus-prime-pro-edition-design-software-version-25-1-for-linux.html)

## Building project

The following setps will generate a qspi image which can be flashed into the PDK

### setup the repository

```bash
git submodule update --init --recursive
```

### Compiling the project
**1. Export the toolchain**
```bash
AARCH64_TOOLCHAINPATH=<toolchain path>
export PATH=$AARCH64_TOOLCHAINPATH/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf/bin/:$PATH
```
**2. Building the sample APPS**

* Hello world sample <br>
    For build steps, check the [hello_world README](FreeRTOS/Demo/CORTEX_A55_AGILEX_5_SOC/apps/hello_world/README.md).
* Driver samples<br>
    For build steps, check the [driver samples README](drivers/samples/README.md).
* OS samples<br>
    For build steps, check the [OS samples README](FreeRTOS/Demo/CORTEX_A55_AGILEX_5_SOC/apps/samples/README.md).

**4. Flashing and running**

For initial device setup, follow this [document](https://altera-fpga.github.io/rel-25.1/embedded-designs/agilex-5/e-series/premium/gsrd/ug-gsrd-agx5e-premium/)<br>
Put the device in JTAG mode to flash the JIC image, refer [Changing MSEL](https://altera-fpga.github.io/rel-25.1/embedded-designs/agilex-5/e-series/premium/gsrd/ug-gsrd-agx5e-premium/#development-kit) section.<br>
After setting the MSEL Turn on the device, Use the following command to flash the firmware to the device.
```bash
#use the image qspi_image.jic
quartus_pgm -c 1 -m jtag -o "piv;<image name>.jic"
```

