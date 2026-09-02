# SoC FPGA FreeRTOS SDK

This repository contains the FreeRTOS port and driver components for SoC FPGA family processors.
This repository includes a few other repositories as submodules.

<details>
<summary><strong>Submodules Included in this repository</strong></summary>

1. **FreeRTOS Kernel**
   [https://github.com/FreeRTOS/FreeRTOS-Kernel.git](https://github.com/FreeRTOS/FreeRTOS-Kernel.git)
   This is the official FreeRTOS Kernel repository.

2. **FreeRTOS-Plus-TCP**
   [https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git](https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git)
   This is the TCP/IP stack from the FreeRTOS community.
   The stack code is taken from the above repository.
   The interface layer and drivers are maintained in this SDK repository.

3. **Arm Trusted Firmware for SoC FPGA**
   [https://github.com/altera-fpga/arm-trusted-firmware.git](https://github.com/altera-fpga/arm-trusted-firmware.git)
   This is the bootloader used for booting FreeRTOS on SoC FPGA.

4. **FCS Library**
   [https://github.com/altera-fpga/libfcs.git](https://github.com/altera-fpga/libfcs.git)
   This is a fork of the FCS library repository.
   The fork includes updates to support FreeRTOS.
   The FCS library provides routines for cryptographic operations in SoC FPGA.

5. **RSU Library**
   [https://github.com/altera-fpga/librsu.git](https://github.com/altera-fpga/librsu.git)
   This is a fork of the Unified Remote System Update (RSU) library.
   The fork includes updates to support FreeRTOS.
   The porting layer is maintained in this SDK repository.
   The RSU library provides routines for remote system updates in SoC FPGA.

6. **TinyUSB**
   [https://github.com/Ignitarium-Technology/tinyusb.git](https://github.com/Ignitarium-Technology/tinyusb.git)
   This is a fork of the TinyUSB stack.
   The fork includes the porting layer for SoC FPGA and updates to support USB 3.
   The driver supporting the SoC FPGA USB 3.1 IP is maintained in this SDK repository.

7. **FAT File System Library**
   [https://github.com/Ignitarium-Technology/Lab-Project-FreeRTOS-FAT.git](https://github.com/Ignitarium-Technology/Lab-Project-FreeRTOS-FAT.git)
   This is a fork of the FreeRTOS+FAT library.
   The fork includes the porting layer to support SoC FPGA.

</details>

<details>
<summary><strong> Directory Structure Overview</strong></summary>

- **FreeRTOS**
  Root directory for FreeRTOS source code, TCP/IP stack, FATFS stack, etc.

- **FreeRTOS/Source**
  Placeholder directory to submodule the FreeRTOS kernel repository.

- **FreeRTOS/portable**
  Root directory to keep the portable layer for the Agilex FreeRTOS port.

- **FreeRTOS/Demo/SOCFPGA**
  Root directory for:
  (i) Kernel test demo applications provided by FreeRTOS
  (ii) Command Line Interface (CLI) application
  (iii) Simple Hello World application
  (iv) Network demo applications, etc.

- **FreeRTOS/Demo/SOCFPGA/startup**
  Startup code for the processor.

- **FreeRTOS/Demo/SOCFPGA/FreeRTOSConfig.h**
  Kernel configuration parameters.

- **FreeRTOS/Demo/SOCFPGA/FreeRTOSIPConfig.h**
  TCP/IP stack configuration parameters.

- **FreeRTOS/Demo/SOCFPGA/apps/hello_world**
  A simple Hello World application.

- **FreeRTOS/Demo/SOCFPGA/apps/samples/cli_app**
  Contains the implementation of a simple Command Line Interface (CLI) application.
  It provides simple commands to exercise different interfaces.

- **FreeRTOS/Demo/SOCFPGA/apps/samples/main_full** and **main_blinky**
  These are the blinky and full test applications provided by FreeRTOS to validate the porting layer.

- **FreeRTOS/Demo/SOCFPGA/apps/samples/main_freertosplus_basic**
  Contains implementations of different applications to demonstrate the network stack.

- **drivers**
  Contains the implementation of drivers for all the hardware blocks.

- **samples**
  Contains sample applications that demonstrate each driver.

- **fcs**
  Placeholder directory to submodule the FCS library repository.

- **rsu**
  Contains the porting layer for RSU and the placeholder directory to submodule the RSU library repository.

- **tinyusb**
  Placeholder directory to submodule the TinyUSB stack.

- **osal**
  Contains the implementation of the Operating System Abstraction Layer.

</details>


For FreeRTOS kernel feature information refer to the
[Developer Documentation](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/00-Developer-docs),
and [API Reference](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/00-TaskHandle).

## Getting started

The following sections describe the quick steps to build and execute using QSPI boot.

## Getting the toolchain

The toolchain can be downloaded from ARM developer [website](https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf.tar.xz).

### Installing the toolchain

Follow the below commands to install the toolchain

```bash
AARCH64_TOOLCHAINPATH=<desired toolchain path>
mkdir -p $AARCH64_TOOLCHAINPATH
cd $AARCH64_TOOLCHAINPATH
cp <download folder>/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf.tar.xz .
tar -xf arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf.tar.xz
rm arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf.tar.xz
export PATH=$AARCH64_TOOLCHAINPATH/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf/bin/:$PATH
```
## Getting Quartus
Follow the steps in the official [website](https://www.intel.com/content/www/us/en/software-kit/865547/intel-quartus-prime-pro-edition-design-software-version-25-3-for-windows.html).<br>
Download and install Quartus

## Building the image

The following steps will generate a qspi image which can be flashed into the PDK

### Set up the repository

```bash
git clone https://github.com/altera-fpga/freertos-socfpga
cd freertos-socfpga
git submodule update --init --recursive
```

### Set up the toolchain and Quartus
- Export Quartus path required to build jic file. **Note** Quartus needs to be exported only if you are planning to build qspi image.
```bash
export QUARTUS_ROOTDIR=~/altera_pro/26.1/quartus/
export PATH=$QUARTUS_ROOTDIR/bin:$QUARTUS_ROOTDIR/linux64:$QUARTUS_ROOTDIR/../qsys/bin:$PATH
```
- Export the toolchain path.
```bash
AARCH64_TOOLCHAINPATH=<toolchain path>
export PATH=$AARCH64_TOOLCHAINPATH/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf/bin/:$PATH
```
### Decide the build options

The following parameters can be specified during the CMake configuration stage:

- **AGILEX5 vs. AGILEX3**<br>
  Select the SOC to build app for, supported SOC's are AGILEX 5 and AGILEX 3
  ```bash
  -DSOC=[AGILEX5|AGILEX3]
  ```
  Build will default to Agilex 5 if not specified or an invalid option is selected

- **Debug vs. Release**<br>
  By default, the build system compiles in **Release** mode. To build a debug version of the application, specify the option:
  ```bash
  -DCMAKE_BUILD_TYPE=Debug
  ```
***configRUN_MULTIPLE_PRIORITIES*** : setting this config to 1 will cause tasks of same priorities to run simultaneously in separate cores

- **A55 vs. A76 Boot**<br>
  By default, the build system compiles for **A55** as the boot core. To tune for A76 boot core version of the application, specify the option:
  ```bash
  -DCORE=A76
  ```

- **SMP (multi-core)**<br>
  By default the build is single-core (`SMP=OFF`). Set `SMP=On` to run FreeRTOS SMP across both cores of the boot core's cluster. The pair is selected automatically from `-DCORE`:
  - `CORE=A55` (default) enables cores **0 and 1** (the two A55 cores).
  - `CORE=A76` enables cores **2 and 3** (the two A76 cores).
  ```bash
  -DSMP=On
  ```
  Using the Makefile:
  ```bash
  make SMP=On <app>            # A55 pair (cores 0,1)
  make SMP=On CORE=A76 <app>   # A76 pair (cores 2,3)
  ```

  > **Note:** A76 SMP requires ATF to hand FreeRTOS off to an A76 core (MPIDR `0x200`) as the primary boot core. If ATF boots on the A55 (MPIDR `0x0`), an A76 SMP build will not come up correctly.

- **HSMP (multi-core)**<br>
  By default the build is single-core (`HSMP=OFF`). Set `HSMP=On` to run FreeRTOS HSMP across cores of different core cluster. 
  ```bash
  -DSMP=On
  ```
  
  Using the Makefile:
  ```bash
  make HSMP=On <app>
  ```

  To disable/enable the cores, manually set the respective core's macro in `FreeRTOSConfigSmp.h` to 0/1 respectively.

`configNUMBER_OF_CORES` is derived automatically from the enabled cores. On Agilex3, cores 2/3 are always disabled.

- **Stack core count (linker)**<br>
  Override the number of core stacks reserved by the linker. This only affects `_NUM_CORE` stack allocation, not which cores are enabled.
  ```bash
  -DCORE_COUNT=<n>
  ```

- **ATF Log Level**<br>
  The default ATF log level is set to `LOG_LEVEL_NOTICE`. To use a different ATF debug log level, specify it using:
  ```bash
  -DATF_LOG_LEVEL=<log-level-value>
  ```

- **SOF File**<br>
  To build SD/eMMC or QSPI image, A .sof file is mandatory.
  The SOF path can be set with:
  ```bash
  -DSOF_PATH=<sof-path>
  ```
- **PFG File**<br>
  To build SD/eMMC or QSPI image, A .pfg file is mandatory.
  Specify a PFG file for either or both according to the build requirements.
  ```bash
  -DPFG_SDMMC=<pfg-file>
  -DPFG_QSPI=<pfg-file>
  ```

### Build the desired application

#### SMP configuration guidance

These settings are present at `FreeRTOS/Demo/SOCFPGA/FreeRTOSConfig.h` and `FreeRTOS/Demo/SOCFPGA/FreeRTOSConfigSmp.h` and control SMP behavior.

- **configNUMBER_OF_CORES**
  - Derived from `configSMP_ENABLED_CORE_COUNT` unless explicitly overridden.

- **configUSE_CORE_AFFINITY**
  - `1` enables task core affinity (pinning tasks to a specific core).
  - Keep `1` when running multiple cores; set `0` only if you want the scheduler to freely migrate tasks.

- **configUSE_PASSIVE_IDLE_HOOK**
  - `0` keeps the standard idle hook behavior.
  - Set to `1` only if you need passive idle hook calls for the secondary idle tasks.

#### Core configuration (SMP)

For the common case, use the **SMP** build option above (`-DSMP=On` / `make SMP=On`). It enables the correct cluster pair for the selected `-DCORE` automatically, so no manual editing is needed.

The underlying controls live in `FreeRTOS/Demo/SOCFPGA/FreeRTOSConfigSmp.h`:

- `configSMP_BUILD` (from `-DSMP`) turns SMP on/off.
- `configBOOT_CORE` (from `-DCORE`: A55 -> 0, A76 -> 2) selects which cluster is paired.
- `configSMP_CORE0_ENABLE` .. `configSMP_CORE3_ENABLE` are then derived from the two values above. Edit this logic only for custom (non-cluster) core combinations.
- MPIDR values can be adjusted via `configSMP_CORE*_MPIDR` if needed.
- Agilex3 supports cores 0 and 1 only; cores 2/3 are forced off when `AGILEX3` is defined.

Resulting enables:

| Build                    | Enabled cores | configNUMBER_OF_CORES |
|--------------------------|---------------|-----------------------|
| default (SMP off)        | boot core only| 1                     |
| `SMP=On`                 | 0, 1 (A55)    | 2                     |
| `SMP=On CORE=A76`        | 2, 3 (A76)    | 2                     |

#### Core configuration (HSMP)

Use the **HSMP** build option (`cmake -DHSMP=On` / `make HSMP=On`), to enable HSMP.

The rest of the controls live in `FreeRTOS/Demo/SOCFPGA/FreeRTOSConfigSmp.h`:

- `configHSMP_BUILD` (from `-DHSMP`) turns HSMP on/off.
- User must manually set macros `configSMP_CORE0_ENABLE` .. `configSMP_CORE3_ENABLE` to enable and disable cores.
- To enable/disable pinning feature, set macro `HET_STRESS_ENABLE_PINNING` in `FreeRTOS/Demo/SOCFPGA/apps/samples/heterogeneous_multi_block_test/heterogeneous_multi_block_test_config.h`.
- MPIDR values can be adjusted via `configSMP_CORE*_MPIDR` if needed.
- Agilex3 supports cores 0 and 1 only; cores 2/3 are forced off when `AGILEX3` is defined.

These are the possible test cases for HSMP:

| Case |       HSMP     | Enabled cores (logical)   | `configSMP_CORE0..3_ENABLE`           | Task pinning (`HET_STRESS_ENABLE_PINNING`) | `configNUMBER_OF_CORES`   |
|----- |--------------- |-------------------------- |-------------------------------------- | ------------------------------------------ | ------------------------- |
| 1    | Off (Default)  | Boot core only            | (default; do not enable extra cores)  | N/A                                        | 1                         |
| 2    | On  (-DHSMP=ON)| 0, 2                      | 1, 0, 1, 0                            | 0 (disabled)                               | 2                         |
| 3    | On  (-DHSMP=ON)| 0, 2                      | 1, 0, 1, 0                            | 1 (enabled)                                | 2                         |
| 4    | On  (-DHSMP=ON)| 0, 3                      | 1, 0, 0, 1                            | 0 (disabled)                               | 2                         |
| 5    | On  (-DHSMP=ON)| 0, 3                      | 1, 0, 0, 1                            | 1 (enabled)                                | 2                         |
| 6    | On  (-DHSMP=ON)| 1, 2                      | 0, 1, 1, 0                            | 0 (disabled)                               | 2                         |
| 7    | On  (-DHSMP=ON)| 1, 2                      | 0, 1, 1, 0                            | 1 (enabled)                                | 2                         |
| 8    | On  (-DHSMP=ON)| 1, 3                      | 0, 1, 0, 1                            | 0 (disabled)                               | 2                         |
| 9    | On  (-DHSMP=ON)| 1, 3                      | 0, 1, 0, 1                            | 1 (enabled)                                | 2                         |
| 10   | On  (-DHSMP=ON)| 0, 1, 2, 3                | 1, 1, 1, 1                            | 0 (disabled)                               | 4                         |
| 11   | On  (-DHSMP=ON)| 0, 1, 2, 3                | 1, 1, 1, 1                            | 1 (enabled)                                | 4                         |


The following applications exist in the repository. Build the application of your choice.
* Hello world application <br>
    For build steps, check the [hello_world README](FreeRTOS/Demo/SOCFPGA/apps/hello_world/README.md).
* Driver samples<br>
    For build steps, check the [driver samples README](samples/README.md).
* CLI Application <br>
        For build steps, check the [cli app README](FreeRTOS/Demo/SOCFPGA/apps/samples/cli_app/README.md).<br>
* Ethernet demo application <br>
        For build steps, check the [enet demo sample README](FreeRTOS/Demo/SOCFPGA/apps/samples/main_freertosplus_basic/README.md).<br>
* FreeRTOS blinky test <br>
        For build steps, check the [Blinky sample README](FreeRTOS/Demo/SOCFPGA/apps/samples/main_blinky/README.md).<br>
* FreeRTOS full test <br>
        For build steps, check the [main_full README](FreeRTOS/Demo/SOCFPGA/apps/samples/main_full/README.md).<br>


### Flash the image and execute

#### Agilex 5
Put the device in JTAG mode to flash the JIC image, refer [Changing MSEL](https://altera-fpga.github.io/rel-25.1/embedded-designs/agilex-5/e-series/premium/gsrd/ug-gsrd-agx5e-premium/#development-kit) section.<br>
After setting the MSEL Turn on the device, Use the following command to flash the firmware to the device.<br>
The .jic images will be in the build folder,
- QSPI boot: <build folder>/qspi_atf_binaries/qspi_image.jic
- SD card boot: <build folder>/sd_atf_binaries/qspi_image.jic
- EMMC boot: <build folder>/emmc_atf_binaries/qspi_image.jic
Navigate to the appropriate folder and run the following command

```bash
#use the image qspi_image.jic
quartus_pgm -c 1 -m jtag -o "piv;qspi_image.jic"
```
Power down the device, set the MSEL back to QSPI mode and then power on to boot the application.
#### Agilex 3
Power on the device and use the following command to flash the firmware to the device
```bash
#use the image qspi_image.jic
quartus_pgm -c 1 -m jtag -o "piv;qspi_image.jic@2"
```
Power cycle the device

### Detailed build steps
Visit tools [README](tools/README.md) for detailed steps on
- Building ATF
- Packaging build artifacts for different boot modes
- Booting in various boot modes
