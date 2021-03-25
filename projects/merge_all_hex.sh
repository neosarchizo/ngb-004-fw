#!/bin/bash
echo "merge all hex"
if test -f "app-settings.hex"; then
    rm app-settings.hex
fi
if test -f "app-settings-bl.hex"; then
    rm app-settings-bl.hex
fi
if test -f "app-settings-bl-sd.hex"; then
    rm app-settings-bl-sd.hex
fi

echo "1) application + settings"
mergehex -m ./spb2-p8/Output/Release/Exe/spb2_p8.hex bootloader_setting.hex -o app-settings.hex
echo "2) 1) + bootloader"
mergehex -m app-settings.hex ./bootloader/Output/Release/Exe/bootloader.hex -o app-settings-bl.hex
echo "3) 2) + SD"
mergehex -m app-settings-bl.hex ../nordic/nRF5_SDK_15.3.0_59ac345/components/softdevice/s112/hex/s112_nrf52_6.1.1_softdevice.hex -o app-settings-bl-sd.hex

echo "done!"

