FROM ubuntu:20.04 as builder

ENV TZ=Asia/Yerevan
ENV DEBIAN_FRONTEND=noninteractive

# disable certificate check (for kitware)
RUN touch /etc/apt/apt.conf.d/99verify-peer.conf && \
    echo >> /etc/apt/apt.conf.d/99verify-peer.conf "Acquire { https::Verify-Peer false }"

# add kitware repo so that fresh cmake could be installed
RUN apt -y update
RUN apt install -y software-properties-common gpg wget flex bison pkg-config
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN apt -y update
RUN apt install --reinstall ca-certificates
RUN apt install kitware-archive-keyring
RUN apt -y purge --auto-remove cmake && apt -y install cmake

# install essential tools
RUN apt -y update && \
    apt -y install clang make cmake python3-pip git

RUN pip3 install meson ninja conan
RUN conan profile new default --detect && \
    conan profile update settings.compiler.libcxx=libstdc++11 default

RUN mkdir -p /home ; cd /home & git clone https://gitlab.freedesktop.org/gstreamer/gstreamer.git && \
    cd gstreamer && \
    meson -Dgpl=enabled build && \
    ninja -C build install

RUN ldconfig

RUN wget https://go.dev/dl/go1.18.3.linux-amd64.tar.gz && \
    tar xf go1.18.3.linux-amd64.tar.gz -C /usr/local/

ENV PATH="${PATH}:/usr/local/go/bin"

RUN apt install -y protobuf-compiler && \
    go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28 && \
    go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2

ENV PATH="${PATH}:/root/go/bin"

WORKDIR /workdir


FROM builder as cpp-builder
COPY src/cpp/conanfile.txt src/cpp/conanfile.txt
RUN cd src/cpp && mkdir build && cd build && conan install .. --build=missing
COPY src/cpp src/cpp
RUN cd src/cpp/build && cmake .. && make -j


FROM cpp-builder as audiomixer
ENV GST_PLUGIN_PATH="/workdir/src/cpp/build/app/audiomixer/gst-plugins/bonaudiomixer"
CMD ["/workdir/src/cpp/build/app/audiomixer/audiomixer"]


FROM cpp-builder as videoscaler
CMD ["/workdir/src/cpp/build/app/videoscaler/videoscaler"]


FROM builder as signaler

COPY go.work go.work
COPY src/go/lib/go.mod src/go/lib/go.mod
COPY src/go/lib/go.sum src/go/lib/go.sum
COPY src/go/app/signaler/go.mod src/go/app/signaler/go.mod
COPY src/go/app/signaler/go.sum src/go/app/signaler/go.sum
RUN cd src/go/lib && go mod vendor
RUN cd src/go/app/signaler && go mod vendor

COPY src/go src/go
COPY src/cpp/app/videoscaler/proto src/cpp/app/videoscaler/proto
COPY src/cpp/app/audiomixer/proto src/cpp/app/audiomixer/proto

RUN cd src/go/app/signaler/transcode && ./generate_stubs.sh
RUN cd src/go/app/signaler && CGO_ENABLED=0 go build
WORKDIR /workdir/src/go/app/signaler
CMD ["./signaler"]
