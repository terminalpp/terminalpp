#!/bin/bash
if tty -s <&1; then
    echo "Output is a tty"
else
    echo "Output is not a tty"
fi