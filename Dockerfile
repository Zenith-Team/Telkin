# FROM devkitpro/devkitppc

FROM ghcr.io/wiiu-env/devkitppc:20241128
WORKDIR /home/user/src

COPY --from=ghcr.io/wiiu-env/libfunctionpatcher:20241012 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libkernel:20230621 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcontentredirection:20250208 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20250208 /artifacts $DEVKITPRO

WORKDIR /github/workspace
COPY --chown=user . /github/workspace
