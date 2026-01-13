#!/usr/bin/python3
#Written by lpereira, modified hwspeedy
#Copyright GPL2+
import json

icons = (
 ("drivers/input/joystick/", "joystick"),
 ("drivers/input/keyboard/", "keyboard"),
 ("drivers/media/usb/uvc/", "camera-web"),
 ("drivers/media/common/videobuf2/", "camera-web"),
 ("drivers/net/wireless/", "wireless"),
 ("drivers/net/ethernet/", "network-interface"),
 ("drivers/input/mouse/", "mouse"),
 ("drivers/bluetooth/", "bluetooth"),
 ("drivers/media/v4l", "camera-web"),
 ("arch/x86/crypto/", "cryptohash"),
 ("drivers/crypto/", "cryptohash"),
 ("net/bluetooth/", "bluetooth"),
 ("drivers/input/", "inputdevices"),
 ("drivers/cdrom/", "cdrom"),
 ("drivers/hwmon/", "therm"),
 ("drivers/thermal/", "therm"),
 ("drivers/iommu/", "memory"),
 ("net/wireless/", "wireless"),
 ("drivers/nvme/", "hdd"),
 ("net/ethernet/", "network-interface"),
 ("drivers/scsi/", "hdd"),
 ("drivers/edac/", "memory"),
 ("drivers/hid/", "inputdevices"),
 ("drivers/gpu/", "monitor"),
 ("drivers/i2c/", "memory"),
 ("drivers/ata/", "hdd"),
 ("drivers/usb/", "usb"),
 ("drivers/pci/", "devices"),
 ("drivers/net/", "network"),
 ("drivers/mmc/", "media-removable"),
 ("drivers/misc/cardreader/", "media-removable"),
 ("drivers/memstick/", "media-removable"),
 ("crypto/", "cryptohash"),
 ("sound/", "audio"),
 ("net/", "network-connections"),
 ("fs/", "filesystem"),
)


icon_list = json.load(open("../data/kernel-module-icons.json", "r"))

for f in open("/tmp/module_list", "r").readlines():
    f = f.strip()
    f = f[len("kernel/"):]
    for path, icon in icons:
        if f.startswith(path):
            kmod = f.split("/")[-1]
            kmod = kmod[:kmod.find(".ko")]
            icon_list[kmod] = icon
            break

print(json.dumps(icon_list, sort_keys=True,indent=4))
