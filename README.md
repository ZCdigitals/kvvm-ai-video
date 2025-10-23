# Video

This is video project in luckfox for kvvm.

## Toolchain

The toolchain is not included in this project. See `https://github.com/ZCdigitals/kvvm-ai-builder`

## Compile

Please compile in docker.

```bash
# build in container
docker run -it --rm -v .:/app builder:luckfox-pico /app/build.sh
```

## Utils

```bash
# set format
v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat=NV12

# capture frame
v4l2-ctl -d /dev/video0 --stream-mmap --stream-to=frame.raw --stream-count=1

# convert frame.raw
ffmpeg -f rawvideo -pixel_format nv12 -video_size 1920x1080 -i frame.raw output.jpg
```
