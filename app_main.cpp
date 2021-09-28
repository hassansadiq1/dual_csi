#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <sstream>

#include "gstnvdsmeta.h"
#include "pipeline.h"

Pipeline csi;

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

void my_handler(int s){
    printf("Caught signal %d\n",s);
    gboolean res = gst_element_send_event(csi.pipeline, gst_event_new_eos());
    if(!res) {
        g_print("Error occurred! EOS signal cannot be sent!\n\r");
    }
    return;
}

int
main (int argc, char *argv[])
{

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    /* Standard GStreamer initialization */
    gst_init (&argc, &argv);

    csi.loop = g_main_loop_new (NULL, FALSE);

    csi.createElements();
    csi.Verify();
    csi.Configure();

    /* we add a message handler */
    csi.bus = gst_pipeline_get_bus (GST_PIPELINE (csi.pipeline));
    csi.bus_watch_id = gst_bus_add_watch (csi.bus, bus_call, csi.loop);
    gst_object_unref (csi.bus);

    csi.ConstructPipeline();

    int num_sources = 2;

    for (int i = 0; i < num_sources; i++){

        GstCapsFeatures *feature1 = NULL;
        GstElement *argusSource = NULL;
        GstElement *capsfilter1;

        gchar argus_name[32] = { }, capsfilter1_name[32];

        g_snprintf (argus_name, 32, "csicam-source-%02d", i);
        g_snprintf (capsfilter1_name, 32, "caps1-%02d", i);
        argusSource = gst_element_factory_make ("nvarguscamerasrc", argus_name);
        capsfilter1 = gst_element_factory_make("capsfilter", capsfilter1_name);

        feature1 = gst_caps_features_new ("memory:NVMM", NULL);

        GstCaps * caps1 = gst_caps_new_simple ("video/x-raw",
                    "format", G_TYPE_STRING, "NV12",
                    "width", G_TYPE_INT, MUXER_OUTPUT_WIDTH, "height", G_TYPE_INT, MUXER_OUTPUT_HEIGHT,
                    "framerate", GST_TYPE_FRACTION,
                    FPS, 1,
                    NULL);

        g_object_set (G_OBJECT (argusSource),
         "sensor-mode", CAMERA_MODE, "sensor-id", i, "bufapi-version", 1,
           NULL);

        gst_caps_set_features (caps1, 0, feature1);
        g_object_set (G_OBJECT (capsfilter1), "caps", caps1, NULL);

        /* we add all elements into the pipeline */
        gst_bin_add_many (GST_BIN (csi.pipeline),argusSource, capsfilter1,
          NULL);

        /* we link the elements together */
        gst_element_link_many (argusSource, capsfilter1,
          NULL);        

        GstPad *sinkpad, *srcpad;
        gchar pad_name[16] = { };

        g_snprintf (pad_name, 15, "sink_%u", i);
        sinkpad = gst_element_get_request_pad (csi.streammux , pad_name);
        if (!sinkpad) {
            g_printerr ("Streammux request sink pad failed. Exiting.\n");
            return -1;
        }

        srcpad = gst_element_get_static_pad (capsfilter1, "src");
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

    g_object_set (G_OBJECT (csi.nvtiler),
     "width", MUXER_OUTPUT_WIDTH*num_sources,
     "height", MUXER_OUTPUT_HEIGHT,
     "rows", 1,
     "columns", num_sources,
      NULL);

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
