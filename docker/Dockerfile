FROM ubuntu:16.04

# Install all needed packages
RUN apt-get update && apt-get install -y \
    build-essential checkinstall yasm gfortran \
    apt-utils software-properties-common \
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

# Install latest git version
RUN apt-add-repository ppa:git-core/ppa \
    && apt-get update \
    && apt-get install -y git

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
            -DBUILD_opencv_java=OFF \
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
COPY r2inference-dependencies-linux-x86_64-v0.1.0.tar.gz /root

RUN tar -C /root -xzf /root/r2inference-dependencies-linux-x86_64-v0.1.0.tar.gz \
 && rm /root/r2inference-dependencies-linux-x86_64-v0.1.0.tar.gz

CMD ["/bin/bash"]
