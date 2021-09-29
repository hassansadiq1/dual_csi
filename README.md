# Introduction 
TODO: Give a short introduction of your project. Let this section explain the objectives or the motivation behind this project. 

## 1.	OS dependencies
 You need to have Jetpack 4.3 or 4.4 or 4.5 installed.  
 CUDA 10.0 or 10.2  
 Deepstream 4 or 5

## 2.	Software dependencies
```bash
     sudo apt-get install -y libcurl3-dev
     sudo apt-get install -y libcpprest-dev
     sudo apt-get install -y libjson-glib-dev
     sudo apt-get install -y libopenblas-dev
     sudo apt-get install -y protobuf-compiler
     sudo apt-get install -y libprotobuf-dev protobuf-compiler
``` 
## 3.	Installation process
In main Folder dual_csi:
```bash
mkdir build
cd ./build
cmake ..
make
```
To run
```bash
./app_main
```
## 4. Application Options
Following options can be modified according to needs in include/pipeline.h'.    
i. Value 1 will show monitor display. Change it 0 to store h265 encoded stream in file.
```
#define STREAMER 1
```
ii. Path to save stream if STREAMER value is 0.
```
#define FILE_PATH "video.mp4"
```
iii. Set bitrate value in which stream is encoded.
```
#define BITRATE 16000000
```
iv. Set different modes of csi cam. For highest resolution i.e. 4k it needs to be set to mode 0. At the start of application, supported modes with respective fps and width/height is shown.
```
#define CAMERA_MODE 0
```
v. Set width and height of incoming stream.
```
#define MUXER_OUTPUT_WIDTH 4032
#define MUXER_OUTPUT_HEIGHT 3040
```

# Contribute
TODO: Explain how other users and developers can contribute to make your code better. 

