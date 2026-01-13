#!/bin/bash

echo "Updating data included in distribution.."

wget -O /tmp/ieeeoui https://standards-oui.ieee.org
cat /tmp/ieeeoui | grep "(base 16)"|sed "s/    (base 16)\t\t//g" |dos2unix|sort  > /tmp/ieeeoui2
cat /tmp/ieeeoui2 | cut -c1-6|tr [:upper:] [:lower:] >/tmp/ieeeouiA
cat /tmp/ieeeoui2 | cut -c7- >/tmp/ieeeouiB
paste -d '' /tmp/ieeeouiA /tmp/ieeeouiB > ../data/ieee_oui.ids
wget -O ../data/pci.ids https://pci-ids.ucw.cz/v2.2/pci.ids
wget -O ../data/usb.ids http://www.linux-usb.org/usb.ids
wget -O ../data/benchmark.json https://hardinfo2.org/benchmark.json?L=250

echo "Done..."


