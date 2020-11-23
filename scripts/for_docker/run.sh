#!/bin/bash -xe
echo 'Removing /liumos/*...'
rm -rf /liumos/* ; true
echo "Syncing to /liumos..."
rsync -avh --exclude='*.img' --exclude='src/third_party*/' --exclude='tools/' --exclude='.git/' /liumos_host/ /liumos/
echo "Syncing third_party_root..."
rsync -avh /prebuilt/liumos/src/third_party_root/ /liumos/src/third_party_root/
echo "Syncing tools..."
rsync -avh /prebuilt/liumos/tools/ /liumos/tools/
echo "Start building..."
cd /liumos # move to the mounted host src dir
make clean
make run_user_headless
