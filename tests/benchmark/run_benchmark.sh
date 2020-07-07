#!/bin/bash

#
# GstInference benchmark script
#
# Run: ./run_benchmark.sh $BACKEND $MODELS_PATH $VIDEO_PATH
# E.g ./run_benchmark.sh tensorflow /home/user/models/tensorflow_models /home/users/test_benchmark_video.mp4
#
# BACKEND could be tensorflow or tflite
#
# MODELS_PATH need to follow this structure for tensorflow
#.
#├── InceptionV1_TensorFlow
#│   ├── graph_inceptionv1_info.txt
#│   ├── graph_inceptionv1_tensorflow.pb
#│   └── imagenet_labels.txt
#├── InceptionV2_TensorFlow
#│   ├── graph_inceptionv2_info.txt
#│   ├── graph_inceptionv2_tensorflow.pb
#│   └── imagenet_labels.txt
#├── InceptionV3_TensorFlow
#│   ├── graph_inceptionv3_info.txt
#│   ├── graph_inceptionv3_tensorflow.pb
#│   └── imagenet_labels.txt
#├── InceptionV4_TensorFlow
#│   ├── graph_inceptionv4_info.txt
#│   ├── graph_inceptionv4_tensorflow.pb
#│   └── imagenet_labels.txt
#├── TinyYoloV2_TensorFlow
#│   ├── graph_tinyyolov2_info.txt
#│   ├── graph_tinyyolov2_tensorflow.pb
#│   └── labels.txt
#└── TinyYoloV3_TensorFlow
#    ├── graph_tinyyolov3_tensorflow.pb
#    ├── graph_tinyyolov3_info.txt
#    └── labels.txt
#
# For TensorFlow-Lite Follow the next PATH structure
#.
#├── InceptionV1_TensorFlow-Lite
#│   ├── graph_inceptionv1_info.txt
#│   ├── graph_inceptionv1.tflite
#│   └── labels.txt
#├── InceptionV2_TensorFlow-Lite
#│   ├── graph_inceptionv2_info.txt
#│   ├── graph_inceptionv2.tflite
#│   └── labels.txt
#├── InceptionV3_TensorFlow-Lite
#│   ├── graph_inceptionv3_info.txt
#│   ├── graph_inceptionv3.tflite
#│   └── labels.txt
#├── InceptionV4_TensorFlow-Lite
#│   ├── graph_inceptionv4_info.txt
#│   ├── graph_inceptionv4.tflite
#│   └── labels.txt
#├── TinyYoloV2_TensorFlow-Lite
#│   ├── graph_tinyyolov2_info.txt
#│   ├── graph_tinyyolov2.tflite
#│   └── labels.txt
#└── TinyYoloV3_TensorFlow-Lite
#    ├── graph_tinyyolov3_info.txt
#    ├── graph_tinyyolov3.tflite
#    └── labels.txt
#
# For edgetpu Follow the next PATH structure
#.
#├── InceptionV1_edgetpu
#│   ├── graph_inceptionv1_info.txt
#│   ├── graph_inceptionv1.tflite
#│   └── labels.txt
#├── InceptionV2_edgetpu
#│   ├── graph_inceptionv2_info.txt
#│   ├── graph_inceptionv2.tflite
#│   └── labels.txt
#├── InceptionV3_edgetpu
#│   ├── graph_inceptionv3_info.txt
#│   ├── graph_inceptionv3.tflite
#│   └── labels.txt
#├── InceptionV4_edgetpu
#│   ├── graph_inceptionv4_info.txt
#│   ├── graph_inceptionv4.tflite
#│   └── labels.txt
#├── TinyYoloV2_edgetpu
#│   ├── graph_tinyyolov2_info.txt
#│   ├── graph_tinyyolov2.tflite
#│   └── labels.txt
#└── TinyYoloV3_edgetpu
#    ├── graph_tinyyolov3_info.txt
#    ├── graph_tinyyolov3.tflite
#    └── labels.txt
#
# For ONNXRT Follow the next PATH structure
#.
#├── InceptionV1_onnxrt
#│   ├── graph_inceptionv1_info.txt
#│   ├── graph_inceptionv1.onnx
#│   └── labels.txt
#├── InceptionV2_onnxrt
#│   ├── graph_inceptionv2_info.txt
#│   ├── graph_inceptionv2.onnx
#│   └── labels.txt
#├── InceptionV3_onnxrt
#│   ├── graph_inceptionv3_info.txt
#│   ├── graph_inceptionv3.onnx
#│   └── labels.txt
#├── InceptionV4_onnxrt
#│   ├── graph_inceptionv4_info.txt
#│   ├── graph_inceptionv4.onnx
#│   └── labels.txt
#├── TinyYoloV2_onnxrt
#│   ├── graph_tinyyolov2_info.txt
#│   ├── graph_tinyyolov2.onnx
#│   └── labels.txt
#└── TinyYoloV3_onnxrt
#    ├── graph_tinyyolov3_info.txt
#    ├── graph_tinyyolov3.onnx
#    └── labels.txt
#
# The output of this script is a CSV (results.csv) with the following structure
# 
#    Name    , FPS ,  CPU
# InceptionV1 , 11 ,  56
# InceptionV2 , 12 ,  57
# InceptionV3 , 14 ,  61
# InceptionV4 , 16 ,  72
# TinyyoloV2  , 11 ,  52
# TinyyoloV3  , 12 ,  62
#

MODELS_PATH=""
BACKEND=""
EXTENSION=""
INTERNAL_PATH=""

# exit when any command fails
set -e

#Script to run each model
run_all_models(){

  model_array=(inceptionv1 inceptionv2 inceptionv3 inceptionv4 tinyyolov2 tinyyolov3)
  model_upper_array=(InceptionV1 InceptionV2 InceptionV3 InceptionV4 TinyYoloV2 TinyYoloV3)
  input_array=(input input input input input/Placeholder inputs )
  output_array=(InceptionV1/Logits/Predictions/Reshape_1 Softmax InceptionV3/Predictions/Reshape_1 
  InceptionV4/Logits/Predictions add_8 output_boxes )

  mkdir -p logs/
  rm -f logs/*

  for ((i=0;i<${#model_array[@]};++i)); do
    echo Perf ${model_array[i]}
    gst-launch-1.0 \
    filesrc location=$VIDEO_PATH num-buffers=600 ! decodebin ! videoconvert ! \
    perf print-arm-load=true name=inputperf ! tee name=t t. ! videoscale ! queue ! net.sink_model t. ! queue ! net.sink_bypass \
    ${model_array[i]} backend=$BACKEND name=net backend::input-layer=${input_array[i]} backend::output-layer=${output_array[i]} \
    model-location="${MODELS_PATH}${model_upper_array[i]}_${INTERNAL_PATH}/graph_${model_array[i]}${EXTENSION}" \
    net.src_bypass ! perf print-arm-load=true name=outputperf ! videoconvert ! fakesink sync=false > logs/${model_array[i]}.log
  done
}

#Script to get data from perf log
get_perf () {

  myvar=$1
  name=$2

  #Read log and filter perf element
  awk '/'$myvar'/&&/timestamp/' logs/${name}.log >  perf.log
  cp perf.log  perf0.log 
  #Filter only the perf that I want
  awk '{gsub(/^.*'$myvar'/,"'$myvar'");print}' perf0.log > perf1.log
  rm perf0.log
  #Filter the columns needed
  awk '{print  $11 "\t" $13}' perf1.log > perf0.log
  rm perf1.log
  #Remove unnecessary characters
  awk '{gsub(/\\/,"");gsub(/\;/,"");gsub(/\"/,"");gsub(/,/,".")}1' perf0.log > perf1.log
  #Save in the directory
  awk 'END{printf "%s", $0}' perf1.log > $myvar/$name.log
  #Get data to save it in the CSV table
  fps=`awk '{printf "%s", $1}' $myvar/$name.log`
  cpu=`awk '{printf "%s", $2}' $myvar/$name.log`

  echo $name, $fps, $cpu >> results.csv

  #Remove unnecessary files
  rm perf.log
  rm perf0.log
  rm perf1.log
}


#Main

if test "$#" -ne 3; then
  if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    echo Usage: ./run_benchmark.sh $BACKEND $MODELS_PATH
    echo E.g: ./run_benchmark.sh tensorflow /home/user/models/tensorflow_models /home/user/test_benchmark_video.mp4
    exit 0
  fi
  if [ "$1" == clean ]
  then
  rm -rf inputperf/
  rm -rf outputperf/
  rm -rf logs/
  else
    echo "Wrong number of parameters"
  fi
  exit 1
fi

#Check for a valid backend
if [ "$1" == tensorflow ] 
then
  EXTENSION="_tensorflow.pb"
  INTERNAL_PATH="TensorFlow"
elif [ "$1" == tflite ]
then
  EXTENSION=".tflite"
  INTERNAL_PATH="TensorFlow-Lite"
elif [ "$1" == edgetpu ]
then
  EXTENSION="_edgetpu.tflite"
  INTERNAL_PATH="edgetpu"
elif [ "$1" == onnxrt ]
then
  EXTENSION=".onnx"
  INTERNAL_PATH="onnxrt"
else
  echo "Invalid Backend"
  exit 1
fi
VIDEO_PATH="$3"
MODELS_PATH="$2"
BACKEND="$1"

#Script execute the models and save perf data.
run_all_models
#Create new csv, table with FPS and CPU per model
rm -f results.csv
touch results.csv
echo "Name, FPS, CPU Load" >> results.csv
echo "InputPerf" >> results.csv

#Get data from inputperf
mkdir -p inputperf
get_perf inputperf inceptionv1
get_perf inputperf inceptionv2
get_perf inputperf inceptionv3
get_perf inputperf inceptionv4
get_perf inputperf tinyyolov2
get_perf inputperf tinyyolov3

#Get data from outputperf
#You can add more perf elements and add
# a block like this, with the new name
mkdir -p outputperf
  echo "OutputPerf" >> results.csv
get_perf outputperf inceptionv1
get_perf outputperf inceptionv2
get_perf outputperf inceptionv3
get_perf outputperf inceptionv4
get_perf outputperf tinyyolov2
get_perf outputperf tinyyolov3
