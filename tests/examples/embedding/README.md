# Embedding Example
This example receives video file, images or camera stream as input and process each frame to detect if a face is among the accepted set of faces. For each processed frame the application captures the signal emitted by GstInference, forwarding the prediction to a placeholder for external logic which computes if the metadata belongs to a valid face. Simultaneously, the pipeline displays the captured frames with the associated label in a window in case of valid face, otherwise a fail label will be displayed.

# Parameters
* -m|--model
Mandatory. Path to the InceptionV4 trained model
* -f|--file
Optional. Path to the video file to be used.
If it is not set, a camera stream will be used.
* -b|--backend
Mandatory. Name of the backed to be used. See Supported Backends for a list of possible options.
* -e|--embeddings
Mandatory. Path to the text file with embeddings data containing valid faces
* -l|--labels
Mandatory. Path to the text file with labels associated to the embeddings file
* -v
Optional. Run verbosely, Can be used to obtain encoded metadata from custom images.
* -h|--help
Displays usage and parameters description 

# Usage
The embeddings and labels files provided for this demo can be found at 'embeddings' folder, those files provide the metadata from the 'images' folder.   
```
./embedding -m <MODEL_PATH> -f <FILE_PATH> -b <BACKEND> -e <EMBEDDINGS_PATH> -l <LABELS_PATH> [-v]
```
# More information
Please check our [developers](https://developer.ridgerun.com/wiki/index.php?title=GstInference/Example_Applications/Embedding) page for this example.
