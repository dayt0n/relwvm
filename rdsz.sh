#!/bin/bash

du -sk ramdisk/fs | awk '{print $1}'

exit 0
