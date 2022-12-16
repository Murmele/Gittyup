FROM alpine
RUN apk add git flatpak-builder bash xvfb-run
RUN flatpak remote-add flathub https://flathub.org/repo/flathub.flatpakrepo
RUN flatpak install --assumeyes runtime/org.kde.Platform/x86_64/5.15-21.08
RUN flatpak install --assumeyes runtime/org.kde.Sdk/x86_64/5.15-21.08
RUN flatpak install --assumeyes runtime/org.freedesktop.Sdk.Extension.golang/x86_64/21.08
