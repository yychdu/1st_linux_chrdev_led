KERNELDIR:=/home/yyc/linux/IMX6ULL/nxp_linux/linux-imx-rel_imx_4.1.15_2.1.0_ga
CURRENT_PATH:=$(shell pwd)

obj-m :=newchrled.o

build : kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:  
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean