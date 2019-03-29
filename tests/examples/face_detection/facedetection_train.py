#! /usr/bin/env python3

import os
import sys
import gi
import cv2
import argparse
import numpy as np
import tensorflow as tf
from os import path
from tensorflow.python.platform import gfile

#FaceNetV1 input size
NETWORK_WIDTH = 160
NETWORK_HEIGHT = 160

def run_inference (session, input_tensor, output_tensor, input_image):
    output = session.run (output_tensor, feed_dict = {input_tensor : input_image})
    return output[0]

def whiten_image (source_image):
    source_mean = np.mean (source_image)
    source_standard_deviation = np.std (source_image)
    std_adjusted = np.maximum (source_standard_deviation, 1.0 / np.sqrt (source_image.size))
    whitened_image = np.multiply(np.subtract (source_image, source_mean), 1 / std_adjusted)
    return whitened_image

def preprocess_image (src):
    preprocessed_image = cv2.resize (src, (NETWORK_WIDTH, NETWORK_HEIGHT))
    preprocessed_image = cv2.cvtColor (preprocessed_image, cv2.COLOR_BGR2RGB)
    preprocessed_image = whiten_image (preprocessed_image)
    return preprocessed_image

def process_images (session, graph, images_dir, input_image_list):
    embeddings = ""
    labels = ""

    #Extract input and output nodes
    input_tensor = graph.get_tensor_by_name ('input:0')
    output_tensor = graph.get_tensor_by_name ('output:0')
    tf.global_variables_initializer ()

    for input_image_file in input_image_list:
        labels = labels + input_image_file[:input_image_file.index(".")] + ";"
        image = cv2.imread (images_dir + input_image_file)
        print ("Using image: " + str (images_dir + input_image_file))
        preprocessed_image = preprocess_image (image)
        input_image = np.reshape (preprocessed_image, (1, 160, 160, 3))
        output = run_inference (session, input_tensor, output_tensor, input_image)
        
        for values in output:
            embeddings = embeddings + str (values) + " "

        embeddings = embeddings[:-1] + ";"

    embeddings = embeddings[:-1]
    labels = labels[:-1]

    embeddings_file = open ("embeddings.txt", "w")
    labels_file = open ("labels.txt", "w")
    embeddings_file.write (embeddings)
    labels_file.write (labels)
    embeddings_file.close ()
    labels_file.close ()

def start_training (model_path, images_dir):
    print ("Starting train mode using images from "+ images_dir)
    with tf.Graph ().as_default () as graph:
      with tf.Session () as session:
        with gfile.FastGFile (model_path,'rb') as f:

          graph_def = tf.GraphDef ()
          graph_def.ParseFromString (f.read ())
          session.graph.as_default ()

          tf.import_graph_def (
            graph_def,
            input_map=None,
            return_elements=None,
            name="",
            op_dict=None,
            producer_op_list=None
          )

          input_image_filename_list = os.listdir (images_dir)
          input_image_filename_list = [i for i in input_image_filename_list if i.endswith ('.jpg')]
          if (len (input_image_filename_list) < 1):
            print ('No .jpg files found')
            return 1

          process_images (session, graph, images_dir, input_image_filename_list)

def main ():
    #Processing arguments
    parser = argparse.ArgumentParser ()
    parser.add_argument ("--model_path", type=str, default="graph_facenetv1_tensorflow.pb", 
                         help="Path to FaceNetV1 .pb model")
    parser.add_argument ("--images_dir", type=str, default="./images/", 
                         help="Path to directory with images to create embeddings, required for train mode")
    args = parser.parse_args ()

    if not (path.exists (args.model_path)):
        print("Path to FaceNetV1 model does not exist")
        return 1
      
    #Images directory to process is required for training
    if not (path.exists (args.images_dir)):
      print ("Path to images folder does not exist")
      return 1

    start_training (args.model_path, args.images_dir)

#Main entry point
if __name__ == "__main__":
    sys.exit (main())
