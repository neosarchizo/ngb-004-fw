#!/bin/bash
echo "generate settings"
nrfutil settings generate --family NRF52810 --application ./spb2-p8/Output/Release/Exe/spb2_p8.hex --application-version 0 --bootloader-version 0 --bl-settings-version 2 bootloader_setting.hex
echo "done!"

