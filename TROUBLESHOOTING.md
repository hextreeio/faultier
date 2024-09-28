# Troubleshooting

You have issues with your Faultier? This is for you!

## First steps

Before you try anything else, update the firmware of the Faultier! You can find the latest firmware [here](https://github.com/hextreeio/faultier/releases).

Download the faultier.uf2 of the latest release, and then connect your Faultier while pressing the "Bootsel" button on the Pico. The Faultier will appear as a mass storage device on your computer. Copy the faultier.uf2 file onto it. **The Faultier will unmount/disappear after this step. This is expected behavior, as it is booting into the new firmware.**

Afterwards, make sure that you have the **latest** version of the Python library. To update it, run:

```
pip3 install -U faultier
```


## MacOS troubleshooting

### OpenOCD can't find faultier.cfg

If OpenOCD can not find faultier.cfg and you are getting a message such as below:

```
# openocd -f interface/faultier.cfg  
Open On-Chip Debugger 0.12.0-01014-g0896f2eff (2024-05-21-12:49) [https://github.com/microsoft/openocd]
Licensed under GNU GPL v2
For bug reports, read
	http://openocd.org/doc/doxygen/bugs.html
embedded:startup.tcl:28: Error: Can't find interface/faultier.cfg
in procedure 'script' 
at file "embedded:startup.tcl", line 28
```

**You are running the wrong or an outdated version of OpenOCD.** Faultier currently requires a forked OpenOCD. You can install it with Homebrew:

```
brew tap stacksmashing/homebrew-tap
brew install --HEAD stacksmashing/tap/openocd-tamarin
```

## Linux troubleshooting

### udev rule

If you can't get OpenOCD to connect to the Faultier and you are getting a permission denied issue, the problem is probably udev.

The Faultier will appear as two USB serial devices (on most machines it will be /dev/ttyUSB0 and /dev/ttyUSB1), but it *also* exposes a vendor-defined USB interface. On Linux, for your regular user to have access to the latter device, you need to create an udev rule. 

Create the file `/etc/udev/rules.d/99-faultier.conf`, and then paste in the following contents:

```
SUBSYSTEM=="usb", ATTR{idVendor}=="2B3E", ATTR{idProduct}=="2343", MODE="0666" 
```

Afterwards, reboot your machine.

### OpenOCD can't find faultier.cfg

If OpenOCD can not find faultier.cfg and you are getting a message such as below:

```
# openocd -f interface/faultier.cfg  
Open On-Chip Debugger 0.12.0-01014-g0896f2eff (2024-05-21-12:49) [https://github.com/microsoft/openocd]
Licensed under GNU GPL v2
For bug reports, read
	http://openocd.org/doc/doxygen/bugs.html
embedded:startup.tcl:28: Error: Can't find interface/faultier.cfg
in procedure 'script' 
at file "embedded:startup.tcl", line 28
```

**You are running the wrong or an outdated version of OpenOCD.** Faultier currently requires a forked OpenOCD, you can download it [here](https://github.com/stacksmashing/openocd-tamarin).

## Windows troubleshooting

### OpenOCD can't find faultier.cfg

If OpenOCD can not find faultier.cfg and you are getting a message such as below:

```
# openocd -f interface/faultier.cfg  
Open On-Chip Debugger 0.12.0-01014-g0896f2eff (2024-05-21-12:49) [https://github.com/microsoft/openocd]
Licensed under GNU GPL v2
For bug reports, read
	http://openocd.org/doc/doxygen/bugs.html
embedded:startup.tcl:28: Error: Can't find interface/faultier.cfg
in procedure 'script' 
at file "embedded:startup.tcl", line 28
```

**You are running the wrong or an outdated version of OpenOCD.** Faultier currently requires a forked OpenOCD, you can download it (and pre-built Windows releases) [here](https://github.com/stacksmashing/openocd-tamarin).
