# Video

This is video project in luckfox for kvvm.

## Doc

https://cdn.static.spotpear.cn/uploads/picture/learn/risc-v/luck-fox-pico/Document/Rockchip_Developer_Guide_MPI_FAQ.pdf

## Toolchain

The toolchain is not included in this project. See `https://github.com/ZCdigitals/kvvm-ai-builder`

## Compile

Please compile in docker.

```bash
# build in container
docker run -it --rm -v .:/app builder:luckfox-pico /app/build.sh
```

## Files

Lib files are copied from `luckfox-pico`.

-   `include`, copied from `<path to luckfox-pico>/media/rockit/out/include`

## Utils

```bash
# set format
v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat=NV12

# capture frame
v4l2-ctl -d /dev/video0 --stream-mmap --stream-to=frame.raw --stream-count=1

# convert frame.raw
ffmpeg -f rawvideo -pixel_format nv12 -video_size 1920x1080 -i frame.raw output.jpg

# `-f 0` is nv12
# `-t 7` is h264
mpi_enc_test -w 1920 -h 1080 -t 7 -i frame.raw -f 0 -o frame.h264
# `-t 8` is jpeg
mpi_enc_test -w 1920 -h 1080 -t 8 -i frame.raw -f 0 -o frame.jpeg

# convert frame.h264
ffmpeg -i frame.h264 output.jpg
```
