<a href="https://developer.ridgerun.com/wiki/index.php?title=GstInference"><img src="https://developer.ridgerun.com/wiki/images/thumb/9/92/GstInference_Logo_with_name.jpeg/600px-GstInference_Logo_with_name.jpeg" height="400" width="400"></a>

# GstInference

>See the **[GstInference wiki](https://developer.ridgerun.com/wiki/index.php?title=GstInference)** for the complete documentation.

GstInference is an open-source project from Ridgerun Engineering that provides a framework for integrating deep learning inference into GStreamer. Either use one of the included elements to do out-of-the box inference using the most popular deep learning architectures, or leverage the base classes and utilities to support your own custom architecture.

This repo uses **[R²Inference](https://github.com/RidgeRun/r2inference)**, an abstraction layer in C/C++ for a variety of machine learning frameworks. With R²Inference a single C/C++ application may work with models on different frameworks. This is useful to execute inference taking advantage of different hardware resources such as CPU, GPU, or  AI optimized acelerators.

GstInference provides several example elements for common applications, such as [`Inception v4`](ext/r2inference/gstinceptionv4.c) for image classification, [`TinyYOLO v2`](ext/r2inference/gsttinyyolov2.c) for object detection, and [`FaceNet`](ext/r2inference/gstfacenetv1.c) for face recognition. Examples are provided for performing inference on any GStreamer video stream.

<img src="https://developer.ridgerun.com/wiki/images/thumb/4/4f/GstInference-examples.jpeg/800px-GstInference-examples.jpeg" width="800">

## Installing GstInference

Follow the steps to get GstInference running on your platform:

* [Clone or download R²Inference](https://github.com/RidgeRun/r2inference)
* [Build R²Inference](https://developer.ridgerun.com/wiki/index.php?title=R2Inference/Getting_started/Building_the_library)
* [Clone or download GstInference](https://github.com/RidgeRun/gst-inference)
* [Build GstInference](https://developer.ridgerun.com/wiki/index.php?title=GstInference/Getting_started/Building_the_plugin)

## Examples

We provide GStreamer [example pipelines](https://developer.ridgerun.com/wiki/index.php?title=GstInference/Example_pipelines) for all our suported platforms,architectures and backends. 

We also provide [example applications](https://developer.ridgerun.com/wiki/index.php?title=GstInference/Example_Applications) for classification, detection and face recognition.

Our [smart lock](tests/examples/face_detection/README.md) example can get you started with a real security camera application.

We also provide example trained models on our [model zoo](https://developer.ridgerun.com/wiki/index.php?title=GstInference/Model_Zoo)
