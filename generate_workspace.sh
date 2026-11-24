#!/bin/bash

###################################################
# Helper script that sets up a eclipse project that
# can be easily imported into RT workbench and used
# for development and debugging.
###################################################

export PNET_REPO=$(pwd)

echo "Repo"  $PNET_REPO

# build dir should be located outside source directory
BUILD_PATH=../workspace_pn

cmake -S $PNET_REPO -B $BUILD_PATH/p_net \
-DCMAKE_BUILD_TYPE=DEBUG \
-DLOG_LEVEL=WARNING \
-DCMAKE_ECLIPSE_MAKE_ARGUMENTS=-j8 \
-DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE \
-G "Eclipse CDT4 - Unix Makefiles" \
-Wno-deprecated
