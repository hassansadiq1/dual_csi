#ifndef PROJECT_PIPELINE_H
#define PROJECT_PIPELINE_H

#include <glib.h>
#include <gst/gst.h>
#include <math.h>
#include <thread>
#include <iostream>
#include "gstnvdsmeta.h"

#define STREAMER 1
#define FPS 30
#define CAMERA_MODE 0
#define MUXER_OUTPUT_WIDTH 4032
#define MUXER_OUTPUT_HEIGHT 3040
#define BITRATE 16000000
#define FILE_PATH "video.mp4"
#define GST_CAPS_FEATURES_NVMM "memory:NVMM"
#define MUXER_BATCH_TIMEOUT_USEC 10000

using namespace std;

class Pipeline
{
public:

    GMainLoop *loop = NULL;
    GstElement *pipeline = NULL, *streammux = NULL, *nvvideoconvert1 = NULL,
     *nvtiler = NULL, *transform = NULL,  *sink = NULL;

    GstElement *nvosd, *nvvideoconvert2, *queue1,
    * encoder = NULL, *parser = NULL, *qtmux = NULL, *filesink = NULL; 
    GstElement *capsfilter1;

    GstBus *bus = NULL;
    guint bus_watch_id;

    void createElements();

    void Verify();

    void Configure();

    void ConstructPipeline();

    void stopPipeline();

    Pipeline();
    
    ~Pipeline();
};

#endif //PROJECT_PIPELINE_H
