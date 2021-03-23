#!/bin/bash
echo "generate settings"
nrfutil settings generate --family NRF52810 --application ./switch/Output/Release/Exe/switch.hex --application-version 1 --bootloader-version 0 --bl-settings-version 2 bootloader_setting.hex
echo "done!"

