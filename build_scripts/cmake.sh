#!/bin/bash
#
set -e

echo ""
echo "---------------------------------------------------------------------------"
echo "-                             mqtt2ecal cmake.sh Release 					-"
echo "---------------------------------------------------------------------------"
if [ ! -d  ../_Release ]; then
	mkdir ../_Release
fi
cd ../_Release
cmake ..


echo ""
echo "---------------------------------------------------------------------------"
echo "-                             mqtt2ecal cmake.sh Debug 					-"
echo "---------------------------------------------------------------------------"
if [ ! -d  ../_Debug ]; then
	mkdir ../_Debug
fi
cd ../_Debug
cmake ..