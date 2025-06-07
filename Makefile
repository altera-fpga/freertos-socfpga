CMAKE_COMMAND = cmake

#Doxygen output path
DOXYGEN_PATH := docs

#demo path for hello world and demo applications
DEMO_PATH := FreeRTOS/Demo/CORTEX_A55_AGILEX_5_SOC/apps
SAMPLES_PATH := drivers/samples/
TESTS_PATH := drivers/tests/

#output directory
HELLO_WORLD_OUT_DIR := build_hello_world
TESTS_OUT_DIR := build_drivertests
DEMO_OUT_DIR := build_demo
SAMPLES_OUT_DIR := build_samples

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

hello_world:
	@$(CMAKE_COMMAND) -S $(DEMO_PATH)/hello_world -B $(HELLO_WORLD_OUT_DIR)
	@make -C $(HELLO_WORLD_OUT_DIR)
	@cp $(HELLO_WORLD_OUT_DIR)/hello_world.elf RTOSApp.elf
	@echo "hello world build completed Successfully.\nOutput Directory : $(HELLO_WORLD_OUT_DIR)"
.PHONY : hello_world

cli:
	@$(CMAKE_COMMAND) -S $(DEMO_PATH)/samples -B $(DEMO_OUT_DIR)
	@make -C $(DEMO_OUT_DIR) RTOSCliApp.elf -j${nproc}
	@cp $(DEMO_OUT_DIR)/RTOSCliApp.elf RTOSApp.elf
	@echo "cli app build completed Successfully.\nOutput Directory : $(DEMO_OUT_DIR)"
.PHONY : cli

os_tests:
	@$(CMAKE_COMMAND) -S $(DEMO_PATH)/samples -B $(DEMO_OUT_DIR)
	@make -C $(DEMO_OUT_DIR) RTOSFullApp.elf -j${nproc}
	@cp $(DEMO_OUT_DIR)/RTOSFullApp.elf RTOSApp.elf
	@echo "os tests build completed Successfully.\nOutput Directory : $(DEMO_OUT_DIR)"
.PHONY : os_tests

blinky:
	@$(CMAKE_COMMAND) -S $(DEMO_PATH)/samples -B $(DEMO_OUT_DIR)
	@make -C $(DEMO_OUT_DIR) RTOSBlinkyApp.elf -j${nproc}
	@cp $(DEMO_OUT_DIR)/RTOSBlinkyApp.elf RTOSApp.elf
	@echo "blinky app build completed Successfully.\nOutput Directory : $(DEMO_OUT_DIR)"
.PHONY : blinky

enet_demo:
	@$(CMAKE_COMMAND) -S $(DEMO_PATH)/samples -B $(DEMO_OUT_DIR)
	@make -C $(DEMO_OUT_DIR) RTOSEnetApp.elf -j${nproc}
	@cp $(DEMO_OUT_DIR)/RTOSEnetApp.elf RTOSApp.elf
	@echo "enet app build completed Successfully.\nOutput Directory : $(DEMO_OUT_DIR)"
.PHONY : enet_demo

drivertests:
	@$(CMAKE_COMMAND) -S $(TESTS_PATH) -B $(TESTS_OUT_DIR)
	@make -C $(TESTS_OUT_DIR) -j${nproc}
	@cp $(TESTS_OUT_DIR)/drivertests.elf RTOSApp.elf
	@echo "Drivertests build completed Successfully.\nOutput Directory : $(TESTS_OUT_DIR)"
.PHONY : drivertests

samples: dma_sample bridge_sample fatfs_sample fpga_sample gpio_sample i2c_sample i3c_sample iossm_sample qspi_sample reboot_manager_sample seu_sample spi_sample timer_sample uart_sample fcs_sample usb_sample wdt_sample
.PHONY : samples

dma_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) dma.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/dma/dma.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : dma_sample

bridge_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) bridge.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/bridge/bridge.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : bridge_sample

fatfs_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) fatfs.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/fatfs/fatfs.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : fatfs_sample

fpga_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) fpga_mngr.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/fpga_manager/fpga_mngr.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : fpga_sample

gpio_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) gpio.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/gpio/gpio.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : gpio_sample

i2c_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) i2c.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/i2c/i2c.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : i2c_sample

i3c_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) i3c.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/i3c/i3c.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : i3c_sample

iossm_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) iossm.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/iossm/iossm.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : iossm_sample

qspi_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) qspi.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/qspi/qspi.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : qspi_sample

reboot_manager_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) reboot_manager.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/reboot_mngr/reboot_manager.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : reboot_manager_sample

sdm_mailbox_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) sdm_mailbox.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/sdm_mailbox/sdm_mailbox.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : sdm_mailbox_sample

sdmmc_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) sdmmc.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/sdmmc/sdmmc.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : sdmmc_sample

seu_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) seu.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/seu/seu.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : seu_sample

spi_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) spi.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/spi/spi.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : spi_sample

timer_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) timer.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/timer/timer.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : timer_sample

uart_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) uart.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/uart/uart.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : uart_sample

fcs_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) fcs.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/fcs/fcs.elf RTOSApp.elf
	echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : fcs_sample

rsu_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) rsu.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/rsu/rsu.elf RTOSApp.elf
	echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : rsu_sample

usb_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) usb.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/usb3/usb.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : usb_sample

wdt_sample:
	@$(CMAKE_COMMAND) -S $(SAMPLES_PATH) -B $(SAMPLES_OUT_DIR)
	@make -C $(SAMPLES_OUT_DIR) wdt.elf -j${nproc}
	@cp $(SAMPLES_OUT_DIR)/wdt/wdt.elf RTOSApp.elf
	@echo "$@ build completed Successfully.\nOutput Directory : $(SAMPLES_OUT_DIR)"
.PHONY : wdt_sample

all: hello_world cli os_tests blinky enet_demo drivertests samples

help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... hello_world"
	@echo "... cli"
	@echo "... enet_demo"
	@echo "... blinky"
	@echo "... os_tests"
	@echo "... drivertests"
	@echo "... samples (builds all the sample applications)"
	@echo "... dma_sample"
	@echo "... bridge_sample"
	@echo "... fatfs_sample"
	@echo "... fpga_manager_sample"
	@echo "... gpio_sample"
	@echo "... i2c_sample"
	@echo "... i3c_sample"
	@echo "... iossm_sample"
	@echo "... qspi_sample"
	@echo "... reboot_manager_sample"
	@echo "... sdm_mailbox_sample"
	@echo "... sdmmc_sample"
	@echo "... seu_sample"
	@echo "... spi_sample"
	@echo "... timer_sample"
	@echo "... uart_sample"
	@echo "... fcs_sample"
	@echo "... usb_sample"
	@echo "... wdt_sample"
	@echo "... sdk_doc (build doxygen docs)"
	@echo "Note : Dont run make with multiple job at once."
.PHONY : help

clean:
	rm -rf $(HELLO_WORLD_OUT_DIR)
	rm -rf $(TESTS_OUT_DIR)
	rm -rf $(DEMO_OUT_DIR)
	rm -rf $(SAMPLES_OUT_DIR)
	rm -rf $(DOXYGEN_PATH)
	rm -rf RTOSApp.elf

sdk_doc:
	doxygen Doxyfile
	@echo "Doxygen build completed.\nDocs generated at : $(DOXYGEN_PATH)"
