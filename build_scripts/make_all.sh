#!/bin/bash
#
set -e

export BUILD_SCRIPTS_PATH=$PWD
. cmake.sh
cd $BUILD_SCRIPTS_PATH
. build.sh