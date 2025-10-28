#!/bin/bash
docker run -it --rm -v .:/app builder:luckfox-pico /app/build.sh
