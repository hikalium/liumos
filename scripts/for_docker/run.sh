#!/bin/bash -xe
echo 'Removing /liumos/*...'
rm -rf /liumos/* ; true
echo "Syncing to /liumos...(tracked files)"
git -C /liumos_host/ ls-files --full-name | \
	rsync -avh \
		--exclude='*.img' \
		--exclude='src/third_party*/' \
		--exclude='tools/' \
		--exclude='.git/' \
		--files-from=- /liumos_host/ /liumos/
echo "Syncing to /liumos... (untracked files)"
git -C /liumos_host/ ls-files --full-name -o --exclude-standard | \
	rsync -avh \
		--exclude='*.img' \
		--exclude='src/third_party*/' \
		--exclude='tools/' \
		--exclude='.git/' \
		--files-from=- /liumos_host/ /liumos/
echo "Deploying prebuilt..."
make prebuilt
echo "Start building..."
cd /liumos # move to the mounted host src dir
make clean
make run GUI=n
