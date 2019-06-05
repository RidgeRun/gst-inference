# Detection Example
This example receives a video file or camera stream to detect objects in each buffer and detects objects of different classes, and their position and size within the frame. For each frame the application captures the signal emitted by GstInference, forwarding the prediction to a placeholder for external logic. Simultaneously, the pipeline displays the captured frames with every detected object marked with a label and a square.

# Parameters
* -m|--model
Mandatory. Path to the InceptionV4 trained model
* -f|--file
Optional. Path to the video file to be used.
If it is not set, a camera stream will be used.
* -b|--backend
Mandatory. Name of the backed to be used. See Supported Backends for a list of possible options.
* -v
Optional. Run verbosely.
* -h|--help
Displays usage and parameters description 

# Usage
```
./detection -m <MODEL_PATH> -f <FILE_PATH> -b <BACKEND> [-v]
```
# More information
Please check our [developers](https://developer.ridgerun.com/wiki/index.php?title=GstInference/Example_Applications/Detection) page for this example.
