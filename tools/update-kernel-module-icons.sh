#!/bin/bash
#Written by hwspeedy
#Copyright GPL2+
bash -c "cd /usr/lib/modules/;find . |grep \.ko |grep kernel |cut -d '/' -f 3- >/tmp/module_list"
python3 update-kernel-module-icons.py >../data/kernel-module-icons.json.NEW
echo "Changes:"
rm -f ../data/kernel-module-icons.json
mv ../data/kernel-module-icons.json.NEW ../data/kernel-module-icons.json
git diff ../data/kernel-module-icons.json
