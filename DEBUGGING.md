# Debugging tips

## usb

- https://github.com/qemu/qemu/blob/master/docs/usb2.txt
```
device_add usb-mouse,id=usbmouse_hotplug,port=4
device_del usbmouse_hotplug
```
