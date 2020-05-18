FROM scratch
ADD ubuntu-xenial-core-cloudimg-amd64-root.tar.gz /
# delete all the apt list files since they're big and get stale quickly
RUN rm -rf /var/lib/apt/lists/*
# this forces "apt-get update" in dependent images, which is also good
# (see also https://bugs.launchpad.net/cloud-images/+bug/1699913)

# a few minor docker-specific tweaks
# see https://github.com/docker/docker/blob/9a9fc01af8fb5d98b8eec0740716226fadb3735c/contrib/mkimage/debootstrap
RUN set -xe \
	\
# https://github.com/docker/docker/blob/9a9fc01af8fb5d98b8eec0740716226fadb3735c/contrib/mkimage/debootstrap#L40-L48
	&& echo '#!/bin/sh' > /usr/sbin/policy-rc.d \
	&& echo 'exit 101' >> /usr/sbin/policy-rc.d \
	&& chmod +x /usr/sbin/policy-rc.d \
	\
# https://github.com/docker/docker/blob/9a9fc01af8fb5d98b8eec0740716226fadb3735c/contrib/mkimage/debootstrap#L54-L56
	&& dpkg-divert --local --rename --add /sbin/initctl \
	&& cp -a /usr/sbin/policy-rc.d /sbin/initctl \
	&& sed -i 's/^exit.*/exit 0/' /sbin/initctl \
	\
# https://github.com/docker/docker/blob/9a9fc01af8fb5d98b8eec0740716226fadb3735c/contrib/mkimage/debootstrap#L71-L78
	&& echo 'force-unsafe-io' > /etc/dpkg/dpkg.cfg.d/docker-apt-speedup \
	\
# https://github.com/docker/docker/blob/9a9fc01af8fb5d98b8eec0740716226fadb3735c/contrib/mkimage/debootstrap#L85-L105
	&& echo 'DPkg::Post-Invoke { "rm -f /var/cache/apt/archives/*.deb /var/cache/apt/archives/partial/*.deb /var/cache/apt/*.bin || true"; };' > /etc/apt/apt.conf.d/docker-clean \
	&& echo 'APT::Update::Post-Invoke { "rm -f /var/cache/apt/archives/*.deb /var/cache/apt/archives/partial/*.deb /var/cache/apt/*.bin || true"; };' >> /etc/apt/apt.conf.d/docker-clean \
	&& echo 'Dir::Cache::pkgcache ""; Dir::Cache::srcpkgcache "";' >> /etc/apt/apt.conf.d/docker-clean \
	\
# https://github.com/docker/docker/blob/9a9fc01af8fb5d98b8eec0740716226fadb3735c/contrib/mkimage/debootstrap#L109-L115
	&& echo 'Acquire::Languages "none";' > /etc/apt/apt.conf.d/docker-no-languages \
	\
# https://github.com/docker/docker/blob/9a9fc01af8fb5d98b8eec0740716226fadb3735c/contrib/mkimage/debootstrap#L118-L130
	&& echo 'Acquire::GzipIndexes "true"; Acquire::CompressionTypes::Order:: "gz";' > /etc/apt/apt.conf.d/docker-gzip-indexes \
	\
# https://github.com/docker/docker/blob/9a9fc01af8fb5d98b8eec0740716226fadb3735c/contrib/mkimage/debootstrap#L134-L151
	&& echo 'Apt::AutoRemove::SuggestsImportant "false";' > /etc/apt/apt.conf.d/docker-autoremove-suggests

# make systemd-detect-virt return "docker"
# See: https://github.com/systemd/systemd/blob/aa0c34279ee40bce2f9681b496922dedbadfca19/src/basic/virt.c#L434
RUN mkdir -p /run/systemd && echo 'docker' > /run/systemd/container

# Install all needed packages
RUN apt-get update && apt-get install -y \
    build-essential checkinstall yasm gfortran \
    apt-utils git \
    python3-pip python3-dev \
    libgstreamer1.0-0 gstreamer1.0-plugins-base \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly gstreamer1.0-libav \
    gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x \
    cmake cpputest automake libtool pkg-config \
    unzip doxygen libgtk2.0-dev \
    libavcodec-dev libavformat-dev libswscale-dev \
    libxine2-dev libv4l-dev \
    libtbb2 libtbb-dev \
    libjpeg8-dev libjasper-dev libpng12-dev \
    libtiff-dev libtiff5-dev \
    libdc1394-22-dev libprotobuf-dev protobuf-compiler \
    libatlas-base-dev libfaac-dev libmp3lame-dev libtheora-dev \
    libvorbis-dev libxvidcore-dev \
    wget vim \
 && rm -rf /var/lib/apt/lists/* \
 && pip3 install meson ninja numpy

# Download and build OpenCV 3.4.10
RUN cd \
    && wget https://github.com/opencv/opencv/archive/3.4.10.zip \
    && unzip 3.4.10.zip \
    && rm 3.4.10.zip \
    && wget https://github.com/opencv/opencv_contrib/archive/3.4.10.zip \
    && unzip 3.4.10.zip \
    && rm 3.4.10.zip \
    && cd opencv-3.4.10 \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=RELEASE \
            -DCMAKE_INSTALL_PREFIX=/usr/local \
            -DINSTALL_C_EXAMPLES=OFF \
            -DBUILD_opencv_python=OFF \
            -DBUILD_opencv_python2=OFF \
            -DBUILD_opencv_python3=OFF \
            -DINSTALL_PYTHON_EXAMPLES=OFF \
            -D BUILD_opencv_java=OFF \
            -DWITH_TBB=ON \
            -DWITH_V4L=OFF \
            -DWITH_QT=OFF \
            -DWITH_OPENGL=OFF \
            -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-3.4.10/modules \
            -DBUILD_EXAMPLES=OFF .. \
    && make -j8 \
    && make install \
    && cd

# Copy tar package with tensorflow and tensorflow lite dependencies
COPY r2inference.tar.gz /root

RUN tar -C /root -xzf /root/r2inference.tar.gz \
 && rm /root/r2inference.tar.gz

CMD ["/bin/bash"]
