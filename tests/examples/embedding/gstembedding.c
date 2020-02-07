/*
 * GStreamer
 * Copyright (C) 2018-2020 RidgeRun <support@ridgerun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <glib-unix.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "customlogic.h"
#include "gst/r2inference/gstinferencemeta.h"

#define GETTEXT_PACKAGE "GstInference"

typedef struct _GstEmbedding GstEmbedding;
struct _GstEmbedding
{
  GstElement *pipeline;
  GMainLoop *main_loop;
  GstElement *inference_element;
  gchar **embeddings_list;
  gchar **labels_list;
  int num_embeddings;
};

GstEmbedding *gst_embedding_new (void);
void gst_embedding_free (GstEmbedding * embedding);
void gst_embedding_create_pipeline (GstEmbedding * embedding);
void gst_embedding_start (GstEmbedding * embedding);
void gst_embedding_stop (GstEmbedding * embedding);
static void gst_embedding_process_inference (GstElement * element,
    GstEmbeddingMeta * model_meta, GstVideoFrame * model_frame,
    GstEmbeddingMeta * bypass_meta, GstVideoFrame * bypass_frame,
    gpointer user_data);
static gboolean gst_embedding_exit_handler (gpointer user_data);
static gboolean gst_embedding_handle_message (GstBus * bus,
    GstMessage * message, gpointer data);
gboolean verbose;
static gchar *gst_embedding_read_file (const gchar * file);

static const gchar *model_path = NULL;
static const gchar *file_path = NULL;
static const gchar *embeddings_path = NULL;
static const gchar *labels_path = NULL;
static const gchar *backend = NULL;

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL},
  {"model", 'm', 0, G_OPTION_ARG_STRING, &model_path, "Model path", NULL},
  {"embeddings", 'e', 0, G_OPTION_ARG_STRING, &embeddings_path,
      "Path to file with encoded valid faces", NULL},
  {"labels", 'l', 0, G_OPTION_ARG_STRING, &labels_path,
      "Path to labels from valid faces", NULL},
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
  GstEmbedding *embedding;
  GMainLoop *main_loop;

  context = g_option_context_new (" - GstInference Embedding Example");
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
    g_print ("Embeddings Path: %s \n", embeddings_path);
    g_print ("Labels Path: %s \n", labels_path);
  }

  if (!backend) {
    g_printerr ("Backend is required for inference (-b <backend>) \n");
    exit (1);
  }

  if (!model_path) {
    g_printerr ("Model path is required (-m <path>) \n");
    exit (1);
  }

  if (!embeddings_path) {
    g_printerr ("Embeddings path is required (-e <path>) \n");
    exit (1);
  }

  if (!labels_path) {
    g_printerr ("Labels path is required (-l <path>) \n");
    exit (1);
  }

  embedding = gst_embedding_new ();
  gst_embedding_create_pipeline (embedding);

  g_unix_signal_add (SIGINT, gst_embedding_exit_handler, embedding);
  embedding->inference_element =
      gst_bin_get_by_name (GST_BIN (embedding->pipeline), "net");

  g_signal_connect (embedding->inference_element, "new-prediction",
      G_CALLBACK (gst_embedding_process_inference), embedding);

  gst_embedding_start (embedding);

  main_loop = g_main_loop_new (NULL, TRUE);
  embedding->main_loop = main_loop;

  g_main_loop_run (main_loop);

  gst_embedding_stop (embedding);
  gst_embedding_free (embedding);

  exit (0);
}

static gboolean
gst_embedding_exit_handler (gpointer data)
{
  GstEmbedding *embedding;

  g_return_val_if_fail (data, FALSE);

  embedding = (GstEmbedding *) data;
  g_main_loop_quit (embedding->main_loop);

  return TRUE;
}

static gchar *
gst_embedding_read_file (const gchar * file)
{

  FILE *infile;
  glong numbytes;
  gchar *buffer;
  size_t bytes_read;

  g_return_val_if_fail (file, NULL);

  infile = fopen (file, "r");
  if (infile == NULL) {
    g_printerr ("The file does not exist \n");
    return NULL;
  }

  fseek (infile, 0L, SEEK_END);
  numbytes = ftell (infile);
  fseek (infile, 0L, SEEK_SET);

  buffer = (gchar *) calloc (numbytes, sizeof (gchar));
  bytes_read = fread (buffer, sizeof (gchar), numbytes - 1, infile);
  if (bytes_read == 0) {
    g_printerr ("The file was not copied \n");
    return NULL;
  }

  fclose (infile);

  return buffer;
}

GstEmbedding *
gst_embedding_new (void)
{
  GstEmbedding *embedding;

  embedding = g_new0 (GstEmbedding, 1);

  return embedding;
}

void
gst_embedding_free (GstEmbedding * embedding)
{

  g_return_if_fail (embedding);

  if (embedding->inference_element) {
    gst_object_unref (embedding->inference_element);
    embedding->inference_element = NULL;
  }

  if (embedding->pipeline) {
    gst_element_set_state (embedding->pipeline, GST_STATE_NULL);
    gst_object_unref (embedding->pipeline);
    embedding->pipeline = NULL;
  }
  if (embedding->main_loop) {
    g_main_loop_unref (embedding->main_loop);
    embedding->main_loop = NULL;
  }

  if (embedding->embeddings_list) {
    g_strfreev (embedding->embeddings_list);
    embedding->embeddings_list = NULL;
  }

  if (embedding->labels_list) {
    g_strfreev (embedding->labels_list);
    embedding->labels_list = NULL;
  }

  g_free (embedding);
}

static void
gst_embedding_process_inference (GstElement * element,
    GstEmbeddingMeta * model_meta, GstVideoFrame * model_frame,
    GstEmbeddingMeta * bypass_meta, GstVideoFrame * bypass_frame,
    gpointer user_data)
{
  GstEmbedding *embedding;

  g_return_if_fail (element);
  g_return_if_fail (model_meta);
  g_return_if_fail (model_frame);
  g_return_if_fail (bypass_meta);
  g_return_if_fail (bypass_frame);
  g_return_if_fail (user_data);

  embedding = (GstEmbedding *) user_data;

  handle_prediction (bypass_frame->data[0], bypass_frame->info.width,
      bypass_frame->info.height, bypass_frame->info.size,
      bypass_meta->embedding, bypass_meta->num_dimensions, verbose,
      embedding->embeddings_list, embedding->labels_list,
      embedding->num_embeddings);
}

void
gst_embedding_create_pipeline (GstEmbedding * embedding)
{
  GString *pipe_desc;
  GstElement *pipeline;
  GError *error = NULL;
  GstBus *bus;
  gchar *embeddings;
  gchar *labels;

  g_return_if_fail (embedding);

  embeddings = gst_embedding_read_file (embeddings_path);
  if (embeddings == NULL) {
    g_printerr ("Error reading embeddings file \n");
    return;
  }

  embedding->embeddings_list = g_strsplit (embeddings, ";", 0);
  embedding->num_embeddings = g_strv_length (embedding->embeddings_list);
  g_strfreev (embedding->embeddings_list);
  embedding->embeddings_list = g_strsplit_set (embeddings, "; ", 0);

  labels = gst_embedding_read_file (labels_path);
  if (labels == NULL) {
    g_printerr ("Error reading labels file \n");
    return;
  }
  embedding->labels_list = g_strsplit (labels, ";", 0);
  pipe_desc = g_string_new ("");

  if (file_path) {
    g_string_append (pipe_desc, " filesrc location=");
    g_string_append (pipe_desc, file_path);
    g_string_append (pipe_desc, " ! decodebin ! ");
  } else {
    g_string_append (pipe_desc, " autovideosrc !  ");
  }

  g_string_append (pipe_desc, " tee name=t t. ! queue ! videoconvert ! ");
  g_string_append (pipe_desc, " videoscale ! net.sink_model t. ! ");
  g_string_append (pipe_desc, " queue ! videoconvert ! net.sink_bypass ");
  g_string_append (pipe_desc, " facenetv1 name=net backend=");
  g_string_append (pipe_desc, backend);
  g_string_append (pipe_desc, " model-location=");
  g_string_append (pipe_desc, model_path);
  if (!strcmp (backend, "tensorflow")) {
    g_string_append (pipe_desc, " backend::input-layer=input ");
    g_string_append (pipe_desc, " backend::output-layer=output ");
  }

  g_string_append (pipe_desc, " net.src_bypass ! videoconvert ! ");
  g_string_append (pipe_desc, " embeddingoverlay embeddings='");
  g_string_append (pipe_desc, embeddings);
  g_string_append (pipe_desc, "' labels='");
  g_string_append (pipe_desc, labels);
  g_string_append (pipe_desc, "' font-scale=1 likeness-threshold=0.9 ");
  g_string_append (pipe_desc, " ! videoconvert ! queue ! ");
  g_string_append (pipe_desc, " autovideosink sync=false");

  g_free (embeddings);
  g_free (labels);

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

  embedding->pipeline = pipeline;

  gst_pipeline_set_auto_flush_bus (GST_PIPELINE (pipeline), FALSE);
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, gst_embedding_handle_message, embedding);
  gst_object_unref (bus);
}

void
gst_embedding_start (GstEmbedding * embedding)
{
  g_return_if_fail (embedding);

  gst_element_set_state (embedding->pipeline, GST_STATE_PLAYING);
}

void
gst_embedding_stop (GstEmbedding * embedding)
{
  g_return_if_fail (embedding);

  gst_element_set_state (embedding->pipeline, GST_STATE_NULL);
}

static void
gst_embedding_handle_eos (GstEmbedding * embedding)
{
  g_return_if_fail (embedding);

  g_main_loop_quit (embedding->main_loop);
}

static void
gst_embedding_handle_error (GstEmbedding * embedding,
    GError * error, const char *debug)
{
  g_return_if_fail (embedding);
  g_return_if_fail (error);
  g_return_if_fail (debug);

  g_printerr ("error: %s\n", error->message);
  g_main_loop_quit (embedding->main_loop);
}

static void
gst_embedding_handle_warning (GstEmbedding * embedding,
    GError * error, const char *debug)
{
  g_return_if_fail (embedding);
  g_return_if_fail (error);
  g_return_if_fail (debug);

  g_printerr ("warning: %s\n", error->message);
}

static void
gst_embedding_handle_info (GstEmbedding * embedding,
    GError * error, const char *debug)
{
  g_return_if_fail (embedding);
  g_return_if_fail (error);
  g_return_if_fail (debug);

  g_print ("info: %s\n", error->message);
}

static gboolean
gst_embedding_handle_message (GstBus * bus, GstMessage * message, gpointer data)
{
  GstEmbedding *embedding;

  g_return_val_if_fail (bus, FALSE);
  g_return_val_if_fail (message, FALSE);
  g_return_val_if_fail (data, FALSE);

  embedding = (GstEmbedding *) data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      gst_embedding_handle_eos (embedding);
      break;
    case GST_MESSAGE_ERROR:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_error (message, &error, &debug);
      gst_embedding_handle_error (embedding, error, debug);
      g_clear_error (&error);
    }
      break;
    case GST_MESSAGE_WARNING:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_warning (message, &error, &debug);
      gst_embedding_handle_warning (embedding, error, debug);
      g_clear_error (&error);
    }
      break;
    case GST_MESSAGE_INFO:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_info (message, &error, &debug);
      gst_embedding_handle_info (embedding, error, debug);
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
