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

#include "gstchildinspector.h"

typedef struct _GstChildInspectorFlag GstChildInspectorFlag;
typedef struct _GstChildInspectorType GstChildInspectorType;

typedef gchar *(*GstChildInspectorTypeToString) (GParamSpec * pspec,
    GValue * value);

static gchar *gst_child_inspector_type_int_to_string (GParamSpec * pspec,
    GValue * value);
static gchar *gst_child_inspector_type_string_to_string (GParamSpec * pspec,
    GValue * value);

struct _GstChildInspectorFlag
{
  gint value;
  const gchar *to_string;
};

struct _GstChildInspectorType
{
  GType value;
  GstChildInspectorTypeToString to_string;
};

static GstChildInspectorFlag flags[] = {
  {G_PARAM_READABLE, "readable"},
  {G_PARAM_WRITABLE, "writable"},
  {}
};

static GstChildInspectorType types[] = {
  {G_TYPE_INT, gst_child_inspector_type_int_to_string},
  {G_TYPE_STRING, gst_child_inspector_type_string_to_string},
  {}
};

static gchar *
gst_child_inspector_type_string_to_string (GParamSpec * pspec, GValue * value)
{
  return g_strdup_printf ("String. Default: \"%s\"",
      g_value_get_string (value));
}

static gchar *
gst_child_inspector_type_int_to_string (GParamSpec * pspec, GValue * value)
{
  GParamSpecInt *pint = G_PARAM_SPEC_INT (pspec);

  return g_strdup_printf ("Integer. Range: %d - %d Default: %d",
      pint->minimum, pint->maximum, g_value_get_int (value));
}

static const gchar *
gst_child_inspector_flag_to_string (GParamFlags flag)
{
  GstChildInspectorFlag *current_flag;
  const gchar *to_string = NULL;

  for (current_flag = flags; current_flag->to_string; ++current_flag) {
    if (current_flag->value == flag) {
      to_string = current_flag->to_string;
      break;
    }
  }

  return to_string;
}

static gchar *
gst_child_inspector_flags_to_string (GParamFlags flags)
{
  guint32 bitflags;
  gint i;
  gchar *sflags = NULL;

  /* Walk through all the bits in the flags */
  bitflags = flags;
  for (i = 31; i >= 0; --i) {
    const gchar *sflag = NULL;
    guint32 bitflag = 0;

    /* Filter the desired bit */
    bitflag = bitflags & (1 << i);
    sflag = gst_child_inspector_flag_to_string (bitflag);

    /* Is this a flag we want to serialize? */
    if (sflag) {
      /* No trailing comma needed on the first case */
      if (!sflags) {
        sflags = g_strdup (sflag);
      } else {
        sflags = g_strdup_printf ("%s, %s", sflag, sflags);
      }
    }
  }

  return sflags;
}

static gchar *
gst_child_inspector_type_to_string (GParamSpec * pspec, GValue * value)
{
  GstChildInspectorType *current_type;
  const GType value_type = G_VALUE_TYPE (value);
  gchar *to_string = NULL;

  for (current_type = types; current_type->to_string; ++current_type) {
    if (current_type->value == value_type) {
      to_string = current_type->to_string (pspec, value);
      break;
    }
  }

  return to_string;
}

gchar *
gst_child_inspector_property_to_string (GObject * object, GParamSpec * param,
    guint alignment)
{
  GValue value = { 0, };
  const gchar *name;
  const gchar *blurb;
  gchar *flags = NULL;
  gchar *type;
  gchar *prop;

  g_return_val_if_fail (param, NULL);
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  name = g_param_spec_get_name (param);
  blurb = g_param_spec_get_blurb (param);

  flags = gst_child_inspector_flags_to_string (param->flags);

  g_value_init (&value, param->value_type);
  if (param->flags & G_PARAM_READABLE) {
    g_object_get_property (object, param->name, &value);
  } else {
    g_param_value_set_default (param, &value);
  }

  type = gst_child_inspector_type_to_string (param, &value);
  g_value_unset (&value);

  prop = g_strdup_printf ("%*s%-20s: %s\n"
      "%*s%-21.21s flags: %s\n"
      "%*s%-21.21s %s",
      alignment, "", name, blurb,
      alignment, "", "", flags, alignment, "", "", type);

  g_free (type);
  g_free (flags);

  return prop;
}


gchar *
gst_child_inspector_properties_to_string (GObject * object, guint alignment,
    gchar * title)
{
  GParamSpec **property_specs;
  guint num_properties, i;
  gchar *prop, *props = NULL;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  property_specs = g_object_class_list_properties
      (G_OBJECT_GET_CLASS (object), &num_properties);

  if (title)
    props = title;

  for (i = 0; i < num_properties; i++) {
    prop =
        gst_child_inspector_property_to_string (object, property_specs[i],
        alignment);
    if (props) {
      gchar *tmp;
      tmp = g_strdup_printf ("%s\n%s", props, prop);
      g_free (prop);
      g_free (props);
      props = tmp;
    } else {
      props = prop;
    }
  }

  g_free (property_specs);

  return props;
}
