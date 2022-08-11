idf_component_register(SRCS 
		  "src/tusb.c"
		  "src/esp32s3.c"
		  "src/common/tusb_fifo.c"
		  "src/device/usbd.c"
		  "src/device/usbd_control.c"
		  "src/class/cdc/cdc_device.c"
		  "src/class/dfu/dfu_rt_device.c"
		  "src/class/hid/hid_device.c"
		  "src/class/midi/midi_device.c"
		  "src/class/msc/msc_device.c"
		  "src/class/net/ecm_rndis_device.c"
		  "src/class/net/ncm_device.c"
		  "src/class/usbtmc/usbtmc_device.c"
		  "src/class/vendor/vendor_device.c"
		  "src/portable/synopsys/dwc2/dcd_dwc2.c"
                    INCLUDE_DIRS "hw" "src" "../../main"
                    REQUIRES main )


INCLUDE(CMakeLists_flags.txt)
