## Prerequisites
* Docker engine. You can find a very detailed installation guide here
https://docs.docker.com/engine/install/ubuntu/

* Download dependencies tarball (needed by Dockerfile)
https://drive.google.com/file/d/1vi_oqMxt_4fQzRE1nnv5abSDIP4Cpv82/view?usp=sharing

The tarball should be placed in the same path as the Dockerfile.

The tarball contains precompiled TensorFlow and TensorFlow Lite libraries 
aswell as some resources needed to run a GStreamer pipeline. 
It should have the following structure:

```
r2inference
├── backends
│   ├── tensorflow
│   │   └── v1.15.0
│   │       └── libtensorflow-cpu-linux-x86_64-1.15.0.tar.gz
│   └── tflite
│       └── v2.0.1
│           ├── binaries
│           │   └── libtensorflow-lite.a
│           └── include
│               └── tensorflow
│                   └── tensorflow
│                       ├── core
│                       └── lite
└── resources
    └── InceptionV1_TensorFlow
        ├── Egyptian_cat.jpg
        ├── graph_inceptionv1_tensorflow.pb
        └── imagenet_labels.txt

```

## Build instructions
`sudo docker build -t r2inference-ubuntu-16.04 -f Dockerfile-Ubuntu-16.04 .`

## Run a container from the image
`sudo docker run -ti -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix r2inference-ubuntu-16.04`

## Building GstInference
For instructions on how to build and test the pluging you can refer to:
* The file .github/workflows/main.yml in this repository
* https://developer.ridgerun.com/wiki/index.php?title=GstInference/Getting_started/Building_the_plugin
* https://developer.ridgerun.com/wiki/index.php?title=R2Inference/Getting_started/Building_the_library

## Notes
* This Dockerfile creates an image that has been succesfully tested to
build r2inference with TensorFlow backend only.
