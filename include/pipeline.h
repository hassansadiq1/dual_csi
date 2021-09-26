#ifndef PROJECT_PIPELINE_H
#define PROJECT_PIPELINE_H

#include <glib.h>
#include <gst/gst.h>
#include <math.h>
#include <thread>
#include <iostream>
#include "gstnvdsmeta.h"

#define STREAMER TRUE
#define GST_CAPS_FEATURES_NVMM "memory:NVMM"
#define MUXER_OUTPUT_WIDTH 1280
#define MUXER_OUTPUT_HEIGHT 720
#define FPS 60
#define MUXER_BATCH_TIMEOUT_USEC 10000

using namespace std;

class Pipeline
{
public:

    GMainLoop *loop = NULL;
    GstElement *pipeline = NULL, *streammux = NULL, *nvvideoconvert1 = NULL,
     *nvtiler = NULL, *transform = NULL,  *sink = NULL;

    GstBus *bus = NULL;
    guint bus_watch_id;
    GstPad *osd_src_pad = NULL;

    guint pgie_batch_size;

    void createElements();

    void Verify();

    void Configure();

    void ConstructPipeline();

    void stopPipeline();

    Pipeline();
    
    ~Pipeline();
};

#endif //PROJECT_PIPELINE_H
