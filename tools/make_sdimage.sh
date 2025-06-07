#!/bin/bash

SCRIPT_PATH="$(dirname $0)"

. $SCRIPT_PATH/argsparse.sh

PART_TYPE="A2"
PART_SIZE_MB=15

argsparse_use_option fip "FIP image" short:f value default:"./fip.bin"
argsparse_use_option out "out image" short:o value default:"./sd.img"
argsparse_use_option sdsz "card size in mb" short:s value type:int default:"20"

argsparse_parse_options "$@"

IMG_FIP="${program_options[fip]}"
IMG_FILE="${program_options[out]}"
IMG_SIZE_MB="${program_options[sdsz]}"

if [ ! -f $IMG_FIP ]; then
    echo "No FIP image found"
    exit 1
fi

# Create empty image
dd if=/dev/zero of="$IMG_FILE" bs=1M count=$IMG_SIZE_MB

fdisk "$IMG_FILE" <<EOF
o
n
p
1


t
$PART_TYPE
w
EOF

start_sector_count=$(fdisk -l $IMG_FILE | awk "/^$IMG_FILE/" | awk '{ print $2 }')

dd if=$IMG_FIP of=$IMG_FILE bs=512 seek=2048 conv=notrunc

