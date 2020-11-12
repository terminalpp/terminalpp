#!/bin/bash
echo -e "ESC\0134: before \033]8;;https://terminalpp.com\033\0134foobar\033]8;;\033\0134 after"
echo -e "BEL:  before \033]8;;https://terminalpp.com\007foobar\033]8;;\007 after"
