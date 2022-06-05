#!/bin/bash

if [[ $# -ne 2 ]]
then
    echo "Illegal number of parameters" >&2
    exit 1
fi

SRC_FILE=$1
SRC_FILE_BASENAME=$(basename $SRC_FILE)

FUNC_NAME=$2

IR_FILE="$SRC_FILE_BASENAME.ll"
OUT_FILE="$SRC_FILE_BASENAME.v"

HLS_BIN_DIR="build/bin"
HLS_SRC_DIR="hls/src"
HLS_OUT_DIR="hls/out"

HLS_TOOL="bitpack-hls"
HLS_IR_FRONTEND="clang"
HLS_IR_FRONTEND_FLAGS="-S -c -emit-llvm -O1 -o $HLS_OUT_DIR/$IR_FILE"

$HLS_IR_FRONTEND $HLS_IR_FRONTEND_FLAGS $SRC_FILE
$HLS_BIN_DIR/$HLS_TOOL $HLS_OUT_DIR/$IR_FILE $FUNC_NAME
./create-dag.sh
