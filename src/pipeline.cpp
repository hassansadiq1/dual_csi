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
    nvvideoconvert1 = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");

    /* Create OSD to draw on the converted RGBA buffer */
    nvtiler = gst_element_factory_make ("nvmultistreamtiler", "nv-tiler");

    /* Finally render the osd output */
    transform = gst_element_factory_make ("nvegltransform", "nvegl-transform");
    sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");
}

void Pipeline::Verify()
{
    if (!pipeline || !streammux) {
        g_printerr ("Initial elements could not be created. Exiting.\n");
        exit(-1);
    }

    if (!nvvideoconvert1 || !nvtiler || !sink) {
        g_printerr ("Pipeline elements could not be created. Exiting.\n");
        exit(-1);
    }

    if(!transform) {
        g_printerr ("One tegra element could not be created. Exiting.\n");
        exit(-1);
    }
}

void Pipeline::Configure()
{
    g_object_set (G_OBJECT (streammux), "batch-size", 2, NULL);

    g_object_set (G_OBJECT (streammux), "width", MUXER_OUTPUT_WIDTH, "height",
        MUXER_OUTPUT_HEIGHT,
        "batched-push-timeout", MUXER_BATCH_TIMEOUT_USEC, NULL);

    g_object_set (G_OBJECT (sink), "qos", 0, "sync", 0, NULL);
}

void Pipeline::ConstructPipeline()
{
    gst_bin_add (GST_BIN (pipeline), streammux);

    /* Set up the pipeline */
    /* we add all elements into the pipeline */
    gst_bin_add_many (GST_BIN (pipeline),
        nvvideoconvert1, nvtiler, transform, sink, NULL);

    /* we link the elements together */
    if (!gst_element_link_many (streammux,
            nvvideoconvert1, nvtiler, transform, sink, NULL)) {
        g_printerr ("Elements could not be linked. Exiting.\n");
        exit(-1);
    }
}

Pipeline::~Pipeline()
{
}
