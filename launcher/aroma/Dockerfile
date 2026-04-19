# FROM devkitpro/devkitppc

FROM ghcr.io/wiiu-env/devkitppc:20260225
WORKDIR /home/user/src

COPY --from=ghcr.io/wiiu-env/libkernel:20260331 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libcontentredirection:20260418 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts $DEVKITPRO

WORKDIR /github/workspace
COPY --chown=user . /github/workspace
