/* GstDetection
 * Copyright (C) 2019 RidgeRun <support@ridgerun.com>
 * All Rights Reserved.
 *
 *The contents of this software are proprietary and confidential to RidgeRun,
 *LLC.  No part of this program may be photocopied, reproduced or translated
 *into another programming language without prior written consent of
 *RidgeRun, LLC.  The user is free to modify the source code after obtaining
 *a software license from RidgeRun.  All source code changes must be provided
 *back to RidgeRun without any encumbrance.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <glib-unix.h>
#include <stdlib.h>
#include <string.h>
#include "gst/r2inference/gstinferencemeta.h"

#include "customlogic.h"

#define GETTEXT_PACKAGE "GstInference"

typedef struct _GstDetection GstDetection;
struct _GstDetection
{
  GstElement *pipeline;
  GMainLoop *main_loop;
  GstElement *inference_element;
};

GstDetection *gst_detection_new (void);
void gst_detection_free (GstDetection * detection);
void gst_detection_create_pipeline (GstDetection * detection);
void gst_detection_start (GstDetection * detection);
void gst_detection_stop (GstDetection * detection);
static void gst_detection_process_inference (GstElement * element,
    GstDetectionMeta * model_meta, GstVideoFrame * model_frame,
    GstDetectionMeta * bypass_meta, GstVideoFrame * bypass_frame,
    gpointer user_data);
static gboolean gst_detection_exit_handler (gpointer user_data);
static gboolean gst_detection_handle_message (GstBus * bus,
    GstMessage * message, gpointer data);
gboolean verbose;
static const gchar *model_path = NULL;
static const gchar *file_path = NULL;
static const gchar *backend = NULL;

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL},
  {"model", 'm', 0, G_OPTION_ARG_STRING, &model_path, "Model path", NULL},
  {"file", 'f', 0, G_OPTION_ARG_STRING, &file_path,
      "File path (or camera, if omitted)", NULL},
  {"backend", 'b', 0, G_OPTION_ARG_STRING, &backend,
      "Backend used for inference, example: tensorflow", NULL},
  {NULL}
};

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  GstDetection *detection;
  GMainLoop *main_loop;

  context = g_option_context_new (" - GstInference Detection Example");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("option parsing failed: %s\n", error->message);
    g_clear_error (&error);
    g_option_context_free (context);
    exit (1);
  }
  g_option_context_free (context);

  if (verbose) {
    g_print ("Model Path: %s \n", model_path);
    g_print ("File path: %s \n", file_path ? file_path : "camera");
    g_print ("Backend: %s \n", backend);
  }

  if (!backend) {
    g_printerr ("Backend is required for inference (-b <backend>) \n");
    exit (1);
  }

  if (!model_path) {
    g_printerr ("Model path is required (-m <path>) \n");
    exit (1);
  }

  detection = gst_detection_new ();
  gst_detection_create_pipeline (detection);

  g_unix_signal_add (SIGINT, gst_detection_exit_handler, detection);
  detection->inference_element =
      gst_bin_get_by_name (GST_BIN (detection->pipeline), "net");

  g_signal_connect (detection->inference_element, "new-prediction",
      G_CALLBACK (gst_detection_process_inference), detection);

  gst_detection_start (detection);

  main_loop = g_main_loop_new (NULL, TRUE);
  detection->main_loop = main_loop;

  g_main_loop_run (main_loop);

  gst_detection_stop (detection);
  gst_detection_free (detection);

  exit (0);
}

static gboolean
gst_detection_exit_handler (gpointer data)
{
  GstDetection *detection;

  g_return_val_if_fail (data, FALSE);

  detection = (GstDetection *) data;
  g_main_loop_quit (detection->main_loop);

  return TRUE;
}

GstDetection *
gst_detection_new (void)
{
  GstDetection *detection;

  detection = g_new0 (GstDetection, 1);

  return detection;
}

void
gst_detection_free (GstDetection * detection)
{

  g_return_if_fail (detection);

  if (detection->inference_element) {
    gst_object_unref (detection->inference_element);
    detection->inference_element = NULL;
  }

  if (detection->pipeline) {
    gst_element_set_state (detection->pipeline, GST_STATE_NULL);
    gst_object_unref (detection->pipeline);
    detection->pipeline = NULL;
  }
  if (detection->main_loop) {
    g_main_loop_unref (detection->main_loop);
    detection->main_loop = NULL;
  }

  g_free (detection);
}

static void
gst_detection_process_inference (GstElement * element,
    GstDetectionMeta * model_meta, GstVideoFrame * model_frame,
    GstDetectionMeta * bypass_meta, GstVideoFrame * bypass_frame,
    gpointer user_data)
{
  gint index;
  PredictionBox *boxes;

  g_return_if_fail (element);
  g_return_if_fail (model_meta);
  g_return_if_fail (model_frame);
  g_return_if_fail (bypass_meta);
  g_return_if_fail (bypass_frame);
  g_return_if_fail (user_data);

  boxes =
      (PredictionBox *) g_malloc (bypass_meta->num_boxes *
      sizeof (PredictionBox));

  for (index = 0; index < bypass_meta->num_boxes; index++) {
    boxes[index].category = bypass_meta->boxes[index].label;
    boxes[index].x = bypass_meta->boxes[index].x;
    boxes[index].y = bypass_meta->boxes[index].y;
    boxes[index].width = bypass_meta->boxes[index].width;
    boxes[index].height = bypass_meta->boxes[index].height;
    boxes[index].probability = bypass_meta->boxes[index].prob;
  }

  handle_prediction (bypass_frame->data[0], bypass_frame->info.width,
      bypass_frame->info.height, bypass_frame->info.size, boxes,
      bypass_meta->num_boxes);
  g_free (boxes);
}

void
gst_detection_create_pipeline (GstDetection * detection)
{
  GString *pipe_desc;
  GstElement *pipeline;
  GError *error = NULL;
  GstBus *bus;

  g_return_if_fail (detection);

  pipe_desc = g_string_new ("");

  g_string_append (pipe_desc, " tinyyolov2 name=net backend=");
  g_string_append (pipe_desc, backend);
  g_string_append (pipe_desc, " model-location=");
  g_string_append (pipe_desc, model_path);
  if (!strcmp (backend, "tensorflow")) {
    g_string_append (pipe_desc, " backend::input-layer=input/Placeholder ");
    g_string_append (pipe_desc, " backend::output-layer=add_8 ");
  }
  if (file_path) {
    g_string_append (pipe_desc, " filesrc location=");
    g_string_append (pipe_desc, file_path);
    g_string_append (pipe_desc, " ! decodebin ! ");
  } else {
    g_string_append (pipe_desc, " autovideosrc ! ");
  }
  g_string_append (pipe_desc,
      " tee name=t t. ! queue ! videoconvert ! videoscale ! ");
  g_string_append (pipe_desc, " net.sink_model t. ! queue ! videoconvert ! ");
  g_string_append (pipe_desc, " video/x-raw,format=RGB ! net.sink_bypass ");
  g_string_append (pipe_desc, " net.src_bypass ! detectionoverlay ! ");
  g_string_append (pipe_desc, " videoconvert ! queue ! ");
  g_string_append (pipe_desc, " autovideosink sync=false ");

  if (verbose)
    g_print ("pipeline: %s\n", pipe_desc->str);

  pipeline = GST_ELEMENT (gst_parse_launch (pipe_desc->str, &error));
  g_string_free (pipe_desc, TRUE);

  if (error) {
    g_print ("pipeline parsing error: %s\n", error->message);
    g_clear_error (&error);
    gst_object_unref (pipeline);
    return;
  }

  detection->pipeline = pipeline;

  gst_pipeline_set_auto_flush_bus (GST_PIPELINE (pipeline), FALSE);
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, gst_detection_handle_message, detection);
  gst_object_unref (bus);
}

void
gst_detection_start (GstDetection * detection)
{
  g_return_if_fail (detection);

  gst_element_set_state (detection->pipeline, GST_STATE_PLAYING);
}

void
gst_detection_stop (GstDetection * detection)
{
  g_return_if_fail (detection);

  gst_element_set_state (detection->pipeline, GST_STATE_NULL);
}

static void
gst_detection_handle_eos (GstDetection * detection)
{
  g_return_if_fail (detection);

  g_main_loop_quit (detection->main_loop);
}

static void
gst_detection_handle_error (GstDetection * detection,
    GError * error, const char *debug)
{
  g_return_if_fail (detection);
  g_return_if_fail (error);
  g_return_if_fail (debug);

  g_printerr ("error: %s\n", error->message);
  g_main_loop_quit (detection->main_loop);
}

static void
gst_detection_handle_warning (GstDetection * detection,
    GError * error, const char *debug)
{
  g_return_if_fail (detection);
  g_return_if_fail (error);
  g_return_if_fail (debug);

  g_printerr ("warning: %s\n", error->message);
}

static void
gst_detection_handle_info (GstDetection * detection,
    GError * error, const char *debug)
{
  g_return_if_fail (detection);
  g_return_if_fail (error);
  g_return_if_fail (debug);

  g_print ("info: %s\n", error->message);
}

static gboolean
gst_detection_handle_message (GstBus * bus, GstMessage * message, gpointer data)
{
  GstDetection *detection;

  g_return_val_if_fail (bus, FALSE);
  g_return_val_if_fail (message, FALSE);
  g_return_val_if_fail (data, FALSE);

  detection = (GstDetection *) data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      gst_detection_handle_eos (detection);
      break;
    case GST_MESSAGE_ERROR:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_error (message, &error, &debug);
      gst_detection_handle_error (detection, error, debug);
      g_clear_error (&error);
    }
      break;
    case GST_MESSAGE_WARNING:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_warning (message, &error, &debug);
      gst_detection_handle_warning (detection, error, debug);
      g_clear_error (&error);
    }
      break;
    case GST_MESSAGE_INFO:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_info (message, &error, &debug);
      gst_detection_handle_info (detection, error, debug);
      g_clear_error (&error);
    }
      break;
    default:
      if (verbose) {
        g_print ("message: %s\n", GST_MESSAGE_TYPE_NAME (message));
      }
      break;
  }

  return TRUE;
}
