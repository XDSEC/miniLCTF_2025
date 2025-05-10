#!/bin/bash

sed -i "s/miniLCTF{test_flag}/$FLAG/g" /app/server.cjs
node server.cjs
