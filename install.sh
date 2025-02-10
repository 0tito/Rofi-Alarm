#!/usr/bin/env bash

#so, i wanna move the script file to .local/bin
#and i wanna compile alarm.cpp, into .local/share/rofi-alarm

echo "Installing..."
USERNAME=$(whoami)

SCRIPTPATH="/home/$USERNAME/.local/bin/rofi-alarm"
FILESPATH="/home/$USERNAME/.local/share/rofi-alarm"
OUTPUT=$FILESPATH"/Alarm"
SOUNDS=$FILESPATH"/Sounds"
DEFAULTSOUND=$SOUNDS"/Alarm.mp3"
mkdir $FILESPATH
mkdir $SOUNDS
g++ -std=c++20 -o $OUTPUT alarm.cpp

mv script.sh $SCRIPTPATH
mv Alarm.mp3 $DEFAULTSOUND

echo "Instalattion complete, you can now delete this folder."