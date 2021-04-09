#!/bin/bash
echo "generate application zip"
nrfutil pkg generate --hw-version 52 --application-version $1 --application ./spb2-p8/Output/Release/Exe/spb2_p8.hex --sd-req 0xB8 --key-file ./bootloader/private.key "app_$1.zip"
echo "done!"