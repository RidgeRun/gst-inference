/* GstClassification
 * Copyright (C) 2019 FIXME <fixme@example.com>
 * Copyright (C) 2010 Entropy Wave Inc
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <stdlib.h>

#include "gst/r2inference/gstinferencemeta.h"

#define GETTEXT_PACKAGE "classification"


typedef struct _GstClassification GstClassification;
struct _GstClassification
{
  GstElement *pipeline;
  GstBus *bus;
  GMainLoop *main_loop;

  GstElement *inference_element;

  gboolean paused_for_buffering;
  guint timer_id;
};

GstClassification *gst_classification_new (void);
void gst_classification_free (GstClassification * classification);
void gst_classification_create_pipeline (GstClassification * classification);
void gst_classification_create_pipeline_playbin (GstClassification *
    classification, const char *uri);
void gst_classification_start (GstClassification * classification);
void gst_classification_stop (GstClassification * classification);

static gboolean gst_classification_handle_message (GstBus * bus,
    GstMessage * message, gpointer data);
static gboolean onesecond_timer (gpointer priv);


gboolean verbose;

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL},

  {NULL}

};

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  GstClassification *classification;
  GMainLoop *main_loop;

  context = g_option_context_new ("- FIXME");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_print ("option parsing failed: %s\n", error->message);
    g_clear_error (&error);
    g_option_context_free (context);
    exit (1);
  }
  g_option_context_free (context);

  classification = gst_classification_new ();

  if (argc > 1) {
    gchar *uri;
    if (gst_uri_is_valid (argv[1])) {
      uri = g_strdup (argv[1]);
    } else {
      uri = g_filename_to_uri (argv[1], NULL, NULL);
    }
    gst_classification_create_pipeline_playbin (classification, uri);
    g_free (uri);
  } else {
    gst_classification_create_pipeline (classification);
  }

  gst_classification_start (classification);

  main_loop = g_main_loop_new (NULL, TRUE);
  classification->main_loop = main_loop;

  g_main_loop_run (main_loop);

  exit (0);
}


GstClassification *
gst_classification_new (void)
{
  GstClassification *classification;

  classification = g_new0 (GstClassification, 1);

  return classification;
}

void
gst_classification_free (GstClassification * classification)
{
  if (classification->inference_element) {
    gst_object_unref (classification->inference_element);
    classification->inference_element = NULL;
  }

  if (classification->pipeline) {
    gst_element_set_state (classification->pipeline, GST_STATE_NULL);
    gst_object_unref (classification->pipeline);
    classification->pipeline = NULL;
  }
  g_free (classification);
}

void
gst_classification_create_pipeline_playbin (GstClassification * classification,
    const char *uri)
{
  GstElement *pipeline;
  GError *error = NULL;

  pipeline = gst_pipeline_new (NULL);
  gst_bin_add (GST_BIN (pipeline),
      gst_element_factory_make ("playbin", "source"));

  if (error) {
    g_print ("pipeline parsing error: %s\n", error->message);
    g_clear_error (&error);
    gst_object_unref (pipeline);
    return;
  }

  classification->pipeline = pipeline;

  gst_pipeline_set_auto_flush_bus (GST_PIPELINE (pipeline), FALSE);
  classification->bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (classification->bus, gst_classification_handle_message,
      classification);

}

static void
process_inference (GstElement * element, gpointer inference_meta,
    GstBuffer * buffer, gpointer user_data)
{

  GstClassificationMeta *class_meta;
  gdouble current;
  gint index;
  gdouble max;

  class_meta = (GstClassificationMeta *) inference_meta;
  index = 0;
  max = -1;

  for (gint i = 0; i < class_meta->num_labels; ++i) {
    current = (class_meta->label_probs)[i];
    if (current > max) {
      max = current;
      index = i;
    }
  }
  g_print ("Highest probability is label %i : (%f) \n", index, max);

}

void
gst_classification_create_pipeline (GstClassification * classification)
{
  GString *pipe_desc;
  GstElement *pipeline;
  GError *error = NULL;

  pipe_desc = g_string_new ("");

  g_string_append (pipe_desc, "inceptionv4 name=net ");
  g_string_append (pipe_desc, "model-location=/home/user/graph.pb ");
  g_string_append (pipe_desc, "backend::input-layer=input ");
  g_string_append (pipe_desc,
      "backend::output-layer=InceptionV4/Logits/Predictions ");
  g_string_append (pipe_desc, "filesrc location=/home/user/cat.mp4 ! ");
  g_string_append (pipe_desc, "decodebin ! videoconvert ! videoscale ! ");
  g_string_append (pipe_desc, "video/x-raw, width=299, heigth=299 ! ");
  g_string_append (pipe_desc, "tee name=t t. ! queue ! videoconvert ! ");
  g_string_append (pipe_desc, "videoscale ! net.sink_model t. ! queue ! ");
  g_string_append (pipe_desc, "net.sink_bypass net.src_bypass ! fakesink");

  if (verbose)
    g_print ("pipeline: %s\n", pipe_desc->str);

  pipeline = (GstElement *) gst_parse_launch (pipe_desc->str, &error);
  g_string_free (pipe_desc, FALSE);

  if (error) {
    g_print ("pipeline parsing error: %s\n", error->message);
    g_clear_error (&error);
    gst_object_unref (pipeline);
    return;
  }

  classification->pipeline = pipeline;

  gst_pipeline_set_auto_flush_bus (GST_PIPELINE (pipeline), FALSE);
  classification->bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (classification->bus, gst_classification_handle_message,
      classification);

  classification->inference_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "net");

  g_signal_connect (classification->inference_element, "new-prediction",
      G_CALLBACK (process_inference), NULL);

}

void
gst_classification_start (GstClassification * classification)
{
  gst_element_set_state (classification->pipeline, GST_STATE_READY);

  classification->timer_id =
      g_timeout_add (1000, onesecond_timer, classification);
}

void
gst_classification_stop (GstClassification * classification)
{
  gst_element_set_state (classification->pipeline, GST_STATE_NULL);

  g_source_remove (classification->timer_id);
}

static void
gst_classification_handle_eos (GstClassification * classification)
{
  gst_classification_stop (classification);
}

static void
gst_classification_handle_error (GstClassification * classification,
    GError * error, const char *debug)
{
  g_print ("error: %s\n", error->message);
  gst_classification_stop (classification);
}

static void
gst_classification_handle_warning (GstClassification * classification,
    GError * error, const char *debug)
{
  g_print ("warning: %s\n", error->message);
}

static void
gst_classification_handle_info (GstClassification * classification,
    GError * error, const char *debug)
{
  g_print ("info: %s\n", error->message);
}

static void
gst_classification_handle_null_to_ready (GstClassification * classification)
{
  gst_element_set_state (classification->pipeline, GST_STATE_PAUSED);

}

static void
gst_classification_handle_ready_to_paused (GstClassification * classification)
{
  if (!classification->paused_for_buffering) {
    gst_element_set_state (classification->pipeline, GST_STATE_PLAYING);
  }
}

static void
gst_classification_handle_paused_to_playing (GstClassification * classification)
{

}

static void
gst_classification_handle_playing_to_paused (GstClassification * classification)
{

}

static void
gst_classification_handle_paused_to_ready (GstClassification * classification)
{

}

static void
gst_classification_handle_ready_to_null (GstClassification * classification)
{
  g_main_loop_quit (classification->main_loop);

}


static gboolean
gst_classification_handle_message (GstBus * bus, GstMessage * message,
    gpointer data)
{
  GstClassification *classification = (GstClassification *) data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      gst_classification_handle_eos (classification);
      break;
    case GST_MESSAGE_ERROR:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_error (message, &error, &debug);
      gst_classification_handle_error (classification, error, debug);
      g_clear_error (&error);
    }
      break;
    case GST_MESSAGE_WARNING:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_warning (message, &error, &debug);
      gst_classification_handle_warning (classification, error, debug);
      g_clear_error (&error);
    }
      break;
    case GST_MESSAGE_INFO:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_info (message, &error, &debug);
      gst_classification_handle_info (classification, error, debug);
      g_clear_error (&error);
    }
      break;
    case GST_MESSAGE_TAG:
    {
      GstTagList *tag_list;

      gst_message_parse_tag (message, &tag_list);
      if (verbose)
        g_print ("tag\n");
    }
      break;
    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState oldstate, newstate, pending;

      gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
      if (GST_ELEMENT (message->src) == classification->pipeline) {
        if (verbose)
          g_print ("state change from %s to %s\n",
              gst_element_state_get_name (oldstate),
              gst_element_state_get_name (newstate));
        switch (GST_STATE_TRANSITION (oldstate, newstate)) {
          case GST_STATE_CHANGE_NULL_TO_READY:
            gst_classification_handle_null_to_ready (classification);
            break;
          case GST_STATE_CHANGE_READY_TO_PAUSED:
            gst_classification_handle_ready_to_paused (classification);
            break;
          case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            gst_classification_handle_paused_to_playing (classification);
            break;
          case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            gst_classification_handle_playing_to_paused (classification);
            break;
          case GST_STATE_CHANGE_PAUSED_TO_READY:
            gst_classification_handle_paused_to_ready (classification);
            break;
          case GST_STATE_CHANGE_READY_TO_NULL:
            gst_classification_handle_ready_to_null (classification);
            break;
          default:
            if (verbose)
              g_print ("unknown state change from %s to %s\n",
                  gst_element_state_get_name (oldstate),
                  gst_element_state_get_name (newstate));
        }
      }
    }
      break;
    case GST_MESSAGE_BUFFERING:
    {
      int percent;
      gst_message_parse_buffering (message, &percent);
      //g_print("buffering %d\n", percent);
      if (!classification->paused_for_buffering && percent < 100) {
        g_print ("pausing for buffing\n");
        classification->paused_for_buffering = TRUE;
        gst_element_set_state (classification->pipeline, GST_STATE_PAUSED);
      } else if (classification->paused_for_buffering && percent == 100) {
        g_print ("unpausing for buffing\n");
        classification->paused_for_buffering = FALSE;
        gst_element_set_state (classification->pipeline, GST_STATE_PLAYING);
      }
    }
      break;
    case GST_MESSAGE_STATE_DIRTY:
    case GST_MESSAGE_CLOCK_PROVIDE:
    case GST_MESSAGE_CLOCK_LOST:
    case GST_MESSAGE_NEW_CLOCK:
    case GST_MESSAGE_STRUCTURE_CHANGE:
    case GST_MESSAGE_STREAM_STATUS:
      break;
    case GST_MESSAGE_STEP_DONE:
    case GST_MESSAGE_APPLICATION:
    case GST_MESSAGE_ELEMENT:
    case GST_MESSAGE_SEGMENT_START:
    case GST_MESSAGE_SEGMENT_DONE:
    case GST_MESSAGE_LATENCY:
    case GST_MESSAGE_ASYNC_START:
    case GST_MESSAGE_ASYNC_DONE:
    case GST_MESSAGE_REQUEST_STATE:
    case GST_MESSAGE_STEP_START:
    case GST_MESSAGE_QOS:
    default:
      if (verbose) {
        g_print ("message: %s\n", GST_MESSAGE_TYPE_NAME (message));
      }
      break;
  }

  return TRUE;
}



static gboolean
onesecond_timer (gpointer priv)
{

  return TRUE;
}



/* helper functions */

#if 0
gboolean
have_element (const gchar * element_name)
{
  GstPluginFeature *feature;

  feature = gst_default_registry_find_feature (element_name,
      GST_TYPE_ELEMENT_FACTORY);
  if (feature) {
    g_object_unref (feature);
    return TRUE;
  }
  return FALSE;
}
#endif
