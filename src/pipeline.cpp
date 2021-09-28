#include "pipeline.h"
#include <unistd.h>

Pipeline::Pipeline()
{
}

void Pipeline::createElements()
{
    /* Create gstreamer elements */
    /* Create Pipeline element that will form a connection of other elements */
    pipeline = gst_pipeline_new ("csi-pipeline");

    /* Create nvstreammux instance to form batches from one or more sources. */
    streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");

    /* Use convertor to convert from NV12 to RGBA as required by nvosd */
    nvvideoconvert1 = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter1");

    /* Create tiler to stich buffers */
    nvtiler = gst_element_factory_make ("nvmultistreamtiler", "nv-tiler");

    queue1 = gst_element_factory_make("queue", "queue-1");

#if STREAMER
    /* Finally render the output */
    transform = gst_element_factory_make ("nvegltransform", "nvegl-transform");
    sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");
#else
    nvvideoconvert2 = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter2");
    nvosd = gst_element_factory_make ("nvdsosd", "nvds-osd");
    capsfilter1 = gst_element_factory_make("capsfilter", "capsfilter-11");
    encoder = gst_element_factory_make ("nvv4l2h265enc", "encoder-1");
    parser = gst_element_factory_make ("h265parse", "parser-1");
    qtmux = gst_element_factory_make ("qtmux", "qtmux-1");
    filesink = gst_element_factory_make ("filesink", "filesink-1");
#endif
}

void Pipeline::Verify()
{
    if (!pipeline || !streammux || !nvvideoconvert1 || !queue1) {
        g_printerr ("Initial elements could not be created. Exiting.\n");
        exit(-1);
    }

#if STREAMER
    if (!nvtiler || !sink) {
        g_printerr ("Streamer elements could not be created. Exiting.\n");
        exit(-1);
    }

    if(!transform) {
        g_printerr ("One tegra element could not be created. Exiting.\n");
        exit(-1);
    }
#else
    if (!nvosd || !nvvideoconvert2 || !capsfilter1 || !encoder || !parser || !qtmux || !filesink) {
        g_printerr ("Encoder elements could not be created. Exiting.\n");
        exit(-1);
    }
#endif
}

void Pipeline::Configure()
{
    g_object_set (G_OBJECT (streammux), "batch-size", 2, NULL);

    g_object_set (G_OBJECT (streammux), "width", MUXER_OUTPUT_WIDTH,
     "height", MUXER_OUTPUT_HEIGHT,
      "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC,
       NULL);

#if STREAMER
    g_object_set (G_OBJECT (sink), "sync", 0, NULL);
#else
    g_object_set (G_OBJECT (encoder),
    "preset-level", 1,
    "maxperf-enable", 1,
    "bufapi-version", 1,
    "insert-sps-pps", 1,
    "bitrate", BITRATE,
    NULL);

    g_object_set (G_OBJECT (filesink), "location", FILE_PATH, NULL);

    GstCapsFeatures *feature1 = gst_caps_features_new ("memory:NVMM", NULL);
    GstCaps * caps1 = gst_caps_new_simple ("video/x-raw",
                "format", G_TYPE_STRING, "NV12",
                NULL);
    gst_caps_set_features (caps1, 0, feature1);
    g_object_set (G_OBJECT (capsfilter1), "caps", caps1, NULL);

#endif

}

void Pipeline::ConstructPipeline()
{
    gst_bin_add (GST_BIN (pipeline), streammux);

    /* Set up the pipeline */
    /* we add all elements into the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), nvtiler, queue1,
#if STREAMER
        transform, sink,
#else
         nvvideoconvert1, nvosd, nvvideoconvert2, encoder, parser, qtmux, filesink,
#endif
        NULL);

    /* we link the elements together */
    if (!gst_element_link_many (streammux, nvtiler,
#if STREAMER
        transform, sink,
#else
        nvvideoconvert1, nvosd, nvvideoconvert2, queue1, encoder, parser, qtmux, filesink,
#endif
        NULL)) {
        g_printerr ("Elements could not be linked. Exiting.\n");
        exit(-1);
    }
}

Pipeline::~Pipeline()
{
}
