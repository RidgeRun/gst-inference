#! /usr/bin/env python3

import os
import sys
import gi
import argparse
from os import path

gi.require_version ('Gst', '1.0')
from gi.repository import Gst, GObject

def enable_pipeline (model_path, embeddings, labels):
    print ("Starting inference using Gstreamer and GstInference")
           
    command = (" gst-launch-1.0 v4l2src device=/dev/video0 !  decodebin  ! tee  name=t t. ! " +
               " videoconvert ! videoscale ! queue ! net.sink_model facenetv1 name=net " + 
               " backend=tensorflow model-location=" + model_path + " backend::input-layer=input " + 
               " backend::output-layer=output t. ! queue ! net.sink_bypass " +
               " net.src_bypass ! videoconvert ! embeddingoverlay labels=\"$(cat labels.txt)\" " +
               " embeddings=\"$(cat embeddings.txt)\" ! videoconvert ! xvimagesink sync=false ")

    print ("Using command: " + command)
    #Executing gstreamer pipeline as terminal command
    os.system (command)

def main ():
    #Processing arguments
    parser = argparse.ArgumentParser ()
    parser.add_argument ("--model_path", type=str, default="graph_facenetv1_tensorflow.pb", 
                         help="Path to FaceNetV1 .pb model")
    parser.add_argument ("--embeddings_path", type=str, default="embeddings.txt", 
                         help="Path to embeddings file created with facenet_train.py")
    parser.add_argument ("--labels_path", type=str, default="labels.txt", 
                         help="Path to labels file created with facenet_train.py")
    args = parser.parse_args ()

    if not (path.exists (args.model_path)):
        print ("Path to FaceNetV1 model does not exist")
        return 1
    
    if not (path.exists(args.embeddings_path)):
        print ("Embeddings file does not exist")
        return 1
  
    if not (path.exists(args.labels_path)):
        print ("Labels file does not exist")
        return 1
    
    #Starting Gstreamer
    Gst.init (None)   
    enable_pipeline (args.model_path, args.embeddings_path, args.labels_path) 

#Main entry point
if __name__ == "__main__":
    sys.exit (main())
