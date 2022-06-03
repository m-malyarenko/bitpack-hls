#!/bin/bash

HLS_OUT_DIR="hls/out"
OUT_TYPE="png"

dot -T $OUT_TYPE -o $HLS_OUT_DIR/dag.$OUT_TYPE $HLS_OUT_DIR/dag.dot
