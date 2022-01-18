#!/bin/bash
#
set -e

echo ""
echo "---------------------------------------------------------------------------"
echo "-                             mqtt2ecal build.sh Release 					-"
echo "---------------------------------------------------------------------------"
cd ../_Release
cmake --build . --config Release


echo ""
echo "---------------------------------------------------------------------------"
echo "-                             mqtt2ecal build.sh Debug 					-"
echo "---------------------------------------------------------------------------"
cd ../_Debug
cmake --build . --config Debug