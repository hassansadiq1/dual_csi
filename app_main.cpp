#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <sstream>

#include "gstnvdsmeta.h"
#include "pipeline.h"

gint frame_number = 0;

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;
    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_EOS:
            g_print ("End of stream\n");
            g_main_loop_quit (loop);
            break;
        case GST_MESSAGE_ERROR:{
            gchar *debug;
            GError *error;
            gst_message_parse_error (msg, &error, &debug);
            g_printerr ("ERROR from element %s: %s\n",
                GST_OBJECT_NAME (msg->src), error->message);
            if (debug)
                g_printerr ("Error details: %s\n", debug);
            g_free (debug);
            g_error_free (error);
            g_main_loop_quit (loop);
            break;
        }
        default:
            break;
    }
  return TRUE;
}


int
main (int argc, char *argv[])
{
    /* Standard GStreamer initialization */
    gst_init (&argc, &argv);

    Pipeline csi;
    csi.loop = g_main_loop_new (NULL, FALSE);

    csi.createElements();
    csi.Verify();
    csi.Configure();

    /* we add a message handler */
    csi.bus = gst_pipeline_get_bus (GST_PIPELINE (csi.pipeline));
    csi.bus_watch_id = gst_bus_add_watch (csi.bus, bus_call, csi.loop);
    gst_object_unref (csi.bus);

    csi.ConstructPipeline();

    int num_sources = 1;

    for (int i = 0; i < num_sources; i++){

        GstCapsFeatures *feature1 = NULL, *feature2 = NULL;
        GstElement *argusSource = NULL, *nvvideoconvert1 = NULL, *videoconvert1 = NULL, *nvvideoconvert2 = NULL;
        GstElement *capsfilter1, *capsfilter2, *capsfilter3;

        gchar argus_name[16] = { }, nvvideoconvert1_name[16] = { },
         videoconvert1_name[16] = { }, nvvideoconvert2_name[16] = { },
          capsfilter1_name[16] = { }, capsfilter2_name[16] = { }, capsfilter3_name[16] = { };

        g_snprintf (argus_name, 15, "csicam-source-%02d", i);
        g_snprintf (nvvideoconvert1_name, 15, "nvc1-%02d", i);
        g_snprintf (videoconvert1_name, 15, "-vidcon1%02d", i);
        g_snprintf (nvvideoconvert2_name, 15, "nvc2-%02d", i);
        g_snprintf (capsfilter1_name, 15, "cap1-%02d", i);
        g_snprintf (capsfilter2_name, 15, "cap2-%02d", i);
        g_snprintf (capsfilter3_name, 15, "cap3-%02d", i);

        argusSource = gst_element_factory_make ("nvarguscamerasrc", argus_name);
        videoconvert1 = gst_element_factory_make ("videoconvert", videoconvert1_name);
        nvvideoconvert1 = gst_element_factory_make ("nvvidconv", nvvideoconvert1_name);
        nvvideoconvert2 = gst_element_factory_make ("nvvideoconvert", nvvideoconvert2_name);
        capsfilter1 = gst_element_factory_make("capsfilter", capsfilter1_name);
        capsfilter2 = gst_element_factory_make("capsfilter", capsfilter2_name);
        capsfilter3 = gst_element_factory_make("capsfilter", capsfilter3_name);

        feature1 = gst_caps_features_new ("memory:NVMM", NULL);
        feature2 = gst_caps_features_new ("memory:NVMM", NULL);

        GstCaps * caps1 = gst_caps_new_simple ("video/x-raw",
                    "format", G_TYPE_STRING, "NV12",
                    "width", G_TYPE_INT, MUXER_OUTPUT_WIDTH, "height", G_TYPE_INT, MUXER_OUTPUT_HEIGHT,
                    "framerate", GST_TYPE_FRACTION,
                    FPS, 1,
                    NULL);
        GstCaps * caps2 = gst_caps_new_simple ("video/x-raw",
                    "format", G_TYPE_STRING, "YUY2",
                    "width", G_TYPE_INT, MUXER_OUTPUT_WIDTH, "height", G_TYPE_INT, MUXER_OUTPUT_HEIGHT,
                    NULL);
        GstCaps * caps3 = gst_caps_new_simple ("video/x-raw",
                    "format", G_TYPE_STRING, "NV12",
                    "width", G_TYPE_INT, MUXER_OUTPUT_WIDTH, "height", G_TYPE_INT, MUXER_OUTPUT_HEIGHT,
                    NULL);

        g_object_set (G_OBJECT (argusSource),
         "sensor-mode", 4, "sensor-id", i,
           NULL);

        gst_caps_set_features (caps1, 0, feature1);
        gst_caps_set_features (caps3, 0, feature2);
        g_object_set (G_OBJECT (capsfilter1), "caps", caps1, NULL);
        g_object_set (G_OBJECT (capsfilter2), "caps", caps2, NULL);
        g_object_set (G_OBJECT (capsfilter3), "caps", caps3, NULL);

        /* we add all elements into the pipeline */
        gst_bin_add_many (GST_BIN (csi.pipeline),argusSource, capsfilter1, nvvideoconvert1,
         capsfilter2, videoconvert1, nvvideoconvert2, capsfilter3,
          NULL);

        /* we link the elements together */
        gst_element_link_many (argusSource, capsfilter1, nvvideoconvert1,
         capsfilter2, videoconvert1, nvvideoconvert2, capsfilter3,
          NULL);        

        GstPad *sinkpad, *srcpad;
        gchar pad_name[16] = { };

        g_snprintf (pad_name, 15, "sink_%u", i);
        sinkpad = gst_element_get_request_pad (csi.streammux , pad_name);
        if (!sinkpad) {
            g_printerr ("Streammux request sink pad failed. Exiting.\n");
            return -1;
        }

        srcpad = gst_element_get_static_pad (capsfilter3, "src");
        if (!srcpad) {
            g_printerr ("Failed to get src pad of source bin. Exiting.\n");
            return -1;
        }

        if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
            g_printerr ("Failed to link source bin to stream muxer. Exiting.\n");
            return -1;
        }

        gst_object_unref (srcpad);
        gst_object_unref (sinkpad);
    }



    // Set the pipeline to "playing" state
    gst_element_set_state(csi.pipeline, GST_STATE_PLAYING);
    g_main_loop_run(csi.loop);
    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (csi.pipeline, GST_STATE_NULL);
    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (csi.pipeline));
    g_source_remove (csi.bus_watch_id);
    g_main_loop_unref (csi.loop);
    return 0;
}
