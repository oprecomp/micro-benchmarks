# kW Demonstrator

This is the source code for the kW demonstrator software and extensions to the virtual platform. It consists of the following parts:

- **boot:** The bootloader that is embedded in the accelerator on a boot ROM. Loads a binary from host memory and executes it.
- **libcxl:** Simulation implementation of libcxl to interface with the virtual platform.
- **liboprecomp:** Library to offload PULP binaries onto the accelerator in conjunction with the bootloader.

Note that these are automatically included with the PULP SDK. So if you're using the SDK, the above is already available.


## Samples

The `samples` directory contains examples on how to write software on the POWER8 and PULP side and how to establish communication via CAPI. See the samples' Makefiles for instructions on how to link against `libcxl` and `liboprecomp`. The following samples are available:

- **samples/nop:** Sends a WED to the PULP side which in turn prints it to the console.
- **samples/square:** Generates a list of random integers and sends them to PULP to calculate their squares, then checks the result. This example includes DMA transfers and the setup of a proper Work Element Descriptor.

Use `make run` in any of the sample directories to build the host and PULP binaries, and run them in the virtual platform. See the Makefiles for details.


## Setup on POWER8

On a fresh POWER8 system, do the following to get a minimal setup that allows you to offload code to a PULP system on an FPGA attached to the machine.

    # install libcxl
    git clone https://github.com/ibm-capi/libcxl.git
    pushd libcxl
    git checkout v1.6
    make
    sudo make install
    popd

    # install liboprecomp
    pushd liboprecomp
    make
    sudo make install
    popd
