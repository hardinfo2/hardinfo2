#!/bin/bash
#Written by hwspeedy
#Copyright GPL2+
bash -c "cd /usr/lib/modules/;find . |grep \.ko |grep kernel |cut -d '/' -f 3- |tr _ - >/tmp/module_list"
python3 update-kernel-module-icons.py >../data/kernel-module-icons.json.NEW
echo "Changes:"
rm -f ../data/kernel-module-icons.json
cat ../data/kernel-module-icons.json.NEW | tr _ - >../data/kernel-module-icons.json
rm -f ../data/kernel-module-icons.json.NEW
git diff ../data/kernel-module-icons.json
