## Face Detection application using Python ##
This folder contains 2 .py file that are able to use a FaceNetV1 model to create embeddings from faces.

** Note **

Before running a Gstreamer pipeline, 3 files are needed:
+embeddings.txt: This file contains embedded metadata related to one o multiple faces.
+labels.txt: This file uses a file name to create labels, this will help to tag each detected face.
+graph_facenetv1_tensorflow.pb: Thisisthe model used for inference.

The graph_facenetv1_tensorflow.pb file can be downloaded from RidgeRun's store, embeddings.txt and labels.txt are generated using the applications provided in this folder.

** facedetection_train.py **
This program will create the embeddings and labels required by facedetection_run.py.

Arguments:
--help: This option will display information related to command line arguments.
--model_path: This option requires a path to a FaceNetV1 model, TensorFlow version.
--images_dir: Thi option requires a path to a folder with images, this folder will be used to create the embeddings.txt and labels.txt files.

* Command line example:
python3 facenet_train.py --model_path /home/graph_facenetv1_tensorflow.pb --images_dir images/

** facedetection_run.py **
This program will create a Gstreamer pipeline using the embeddings and labels files created in this directory by facedetection_train.py.
The pipeline will use a camera in the computer to feed the inference network, the embeddingoverlay plugin will overlay the streaming with a tag when a valid face is detected.
The valid faces will be stored in the images folder used by facedetection_train.py.

Arguments:
--help: This option will display information related to command line arguments.
--model_path: This option requires a path to a FaceNetV1 model, TensorFlow version.
--embeddings_path: This option requires a path to a TXT file containing embedded metadata.
--labels_path: This option requires a path to a TXT file containing the labels used to create the embedding metadata..

* Command line example:
python3 facenet_run.py --model_path /home/graph_facenetv1_tensorflow.pb --embeddings_path embeddings.txt --labels labels.txt

