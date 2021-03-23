#!/bin/bash
echo "generate application zip"
nrfutil pkg generate --hw-version 52 --application-version $1 --application ./switch/Output/Release/Exe/switch.hex --sd-req 0xCD --key-file ./bootloader/private.key "app_$1.zip"
echo "done!"