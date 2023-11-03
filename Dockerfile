FROM registry.opensuse.org/opensuse/leap:15.4 AS base

RUN zypper --non-interactive ar \
  https://download.opensuse.org/repositories/filesystems:/ceph:/s3gw/15.4/ s3gw \
 && zypper --gpg-auto-import-keys ref

# Needed to match sqlite3 version in s3gw's Dockerfile
RUN zypper --non-interactive install \
  libsqlite3-0=3.43.1 \
 && zypper clean --all

FROM base as build

RUN zypper --non-interactive install \
  binutils \
  clang \
  cmake \
  gcc11 \
  gcc11-c++ \
  libboost_program_options1_80_0-devel \
  libstdc++6-devel-gcc11 \
  sqlite3-devel \
 && zypper clean --all

COPY src /src
WORKDIR /src
RUN cmake -B /build /src \
 && cmake --build /build

FROM base as fsck.s3gw

RUN zypper --non-interactive install \
  libboost_program_options1_80_0 \
  sqlite3 \
 && zypper clean --all

COPY --from=build /build/fsck.s3gw /usr/bin/fsck.s3gw

VOLUME /volume
ENTRYPOINT [ "/usr/bin/fsck.s3gw" ]
CMD [ "/volume" ]
