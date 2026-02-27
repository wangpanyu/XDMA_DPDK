# DPDK XDMA Driver
## 1. Setup: Download and modifications

The driver code requires DPDK version 22.11.

Create a directory for the DPDK download on the server where the FPGA
is installed and move to this directory.
``` shell
git clone http://dpdk.org/git/dpdk-stable
cd dpdk-stable
git checkout v22.11
git clone git://dpdk.org/dpdk-kmods
cp -r <dpdk_sw_database>/drivers ./drivers/net/
cp -r <dpdk_sw_database>/examples ./drivers/examples/
```

Additionally, make below changes to the DPDK 23.11 tree to build XDMA driver

1.  update below configurations	in `./config/rte_config.h`
``` C
CONFIG_RTE_MAX_ETHPORTS=256
CONFIG_RTE_MAX_QUEUES_PER_PORT=4
```
2. Add below lines to `./config/meson.build` in DPDK 23.11 tree
``` C
# Set maximum Ethernet ports to 256
dpdk_conf.set('RTE_MAX_ETHPORTS', 256)
```
3. Add below lines to `./config/rte_config.h` to enable driver debug logs

if you need not to use debug mode, skip this step

``` C
#define RTE_LIBRTE_XDMA_DEBUG_DRIVER 1
```
and make change in `dpdk-stable/meson.build`
``` C
project('DPDK', 'C',
        # Get version number from file.
        # Fallback to "more" for Windows compatibility.
        version: run_command(find_program('cat', 'more'),
            files('VERSION'), check: true).stdout().strip(),
        license: 'BSD',
        default_options: [
            'buildtype=release',
            'default_library=static',
            'warning_level=2',
        ],
        meson_version: '>= 0.53.2'
)
```
change `release` to `debug`

4. Add below line to `./drivers/net/meson.build`, where PMDs are added to drivers list
``` C
'xdma',
```
5. To add Xilinx devices for device binding, add below lines to	`./usertools/dpdk-devbind.py` after cavium_pkx class, where PCI base class for devices are listed.
``` python
xilinx_xdma = {'Class':  '05', 'Vendor': '10ee', 'Device': '9011,9111,9211,9311,9014,9114,9214,9314,9018,9118,9218,9318,901f,911f,921f,931f,9021,9121,9221,9321,9024,9124,9224,9324,9028,9128,9228,9328,902f,912f,922f,932f,9031,9131,9231,9331,9034,9134,9234,9334,9038,9138,9238,9338,903f,913f,923f,933f,9041,9141,9241,9341,9044,9144,9244,9344,9048,9148,9248,9348,b011,b111,b211,b311,b014,b114,b214,b314,b018,b118,b218,b318,b01f,b11f,b21f,b31f,b021,b121,b221,b321,b024,b124,b224,b324,b028,b128,b228,b328,b02f,b12f,b22f,b32f,b031,b131,b231,b331,b034,b134,b234,b334,b038,b138,b238,b338,b03f,b13f,b23f,b33f,b041,b141,b241,b341,b044,b144,b244,b344,b048,b148,b248,b348',
				'SVendor': None, 'SDevice': None}
```
6. Update entries in network devices class in ./usertools/dpdk-devbind.py to add Xilinx devices
``` python
network_devices = [network_class, cavium_pkx, xilinx_xdma],
```

## 2.Setup: Host system
1. TO add hugepages for DPDK, add following parameter to `/etc/default/grub` file
``` C
GRUB_CMDLINE_LINUX="default_hugepagesz=1GB hugepagesz=1G hugepages=5"
```
2. Execute the following command to modify the /boot/grub/grub.cfg with the configuration set in the above step and permanently add them to the kernel command line.
``` C
update-grub
```
3. Reboot host system after making the above modifications.

## 3.Setup: Build Commands
1. Compile DPDK & XDMA driver

Execute the following to compile and install the driver.
``` shell
sudo su
apt install python3-pyelftools libnuma-dev pkg-config
cd dpdk-stable
meson build
cd build
ninja
ninja install
ldconfig
```

The following should appear when ninja completes
```
Linking target app/test/dpdk-test.
```
Verify that `librte_net_xdma.a` is installed in `./build/drivers` directory.
Execute the following to compile the igb_uio kernel driver.
```
cd dpdk-stable/dpdk-kmods/linux/igb_uio
make
```
## 4.Compile Test application
1. Change to root user and compile the application
``` shell
sudo su
cd examples/xdma_testapp
make RTE_SDK=`pwd`/../.. RTE_TARGET=build
```
The following should appear when make completes
``` shell
ln -sf xdma_testapp-shared build/xdma_testapp
```

## 5.Running the DPDK software test application
1. Navigate to examples/xdma_testapp directory.
``` shell
cd dpdk-stable/examples/xdma_testapp
```
2. Run the 'lspci' command on the console and verify that the devices are detected as shown below.
``` shell
$ lspci | grep Xilinx
3b:00.0 Memory controller: Xilinx Corporation Device 903f
```
3. Execute the following commands required for running the DPDK application
``` shell
mkdir /mnt/huge
mount -t hugetlbfs nodev /mnt/huge
modprobe uio
insmod dpdk-stable/dpdk-kmods/linux/igb_uio/igb_uio.ko
```
4. Bind ports to the igb_uio module as shown below
``` shell
../../usertools/dpdk-devbind.py -b igb_uio 18:00.0
../../usertools/dpdk-devbind.py -b igb_uio 3b:00.0
```
Run the command
``` shell
$ lspci -s 3b:00.0 -v
```
if you see
``` shell
Kernel driver in use: igb_uio
```
the port has banded to igb_uio successfully

5. Run the xdma_testapp using the following command
``` shell
./build/xdma_testapp
```