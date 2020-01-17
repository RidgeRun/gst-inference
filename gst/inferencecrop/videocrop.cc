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

#include "videocrop.h"

#include <iostream>
#include <math.h>

const std::string&
VideoCrop::GetFactory () const {
  return this->factory;
}

void
VideoCrop::UpdateElement (GstElement *element,
                          gint image_width,
                          gint image_height,
                          gint x,
                          gint y,
                          gint width,
                          gint height,
                          gint width_ratio,
                          gint height_ratio) {
  gint top = y;
  gint bottom = image_height - y - height;
  gint left = x;
  gint right = image_width - x - width;

  if (width_ratio > 0 && height_ratio > 0) {
    gint top_bottom_modify = round(((height_ratio * width) / width_ratio - height) /
                                   2);
    gint left_right_modify = round(((width_ratio * height) / height_ratio - width) /
                                   2);
    if (width_ratio <= height_ratio) {
      if (width > height) {
        top = top - top_bottom_modify;
        bottom = bottom - top_bottom_modify;
      } else {
        left = left - left_right_modify;
        right = right - left_right_modify;
      }
    } else {
      if (width < height) {
        left = left - left_right_modify;
        right = right - left_right_modify;
      } else {
        top = top - top_bottom_modify;
        bottom = bottom - top_bottom_modify;
      }
    }
  }

  if (top < 0) {
    top = 0;
  }

  if (bottom < 0) {
    bottom = 0;
  }

  if (left < 0) {
    left = 0;
  }

  if (right < 0) {
    right = 0;
  }

  g_return_if_fail (element);

  g_object_set (element,
                "top", top,
                "bottom", bottom,
                "left", left,
                "right", right, NULL);
}

GstPad *
VideoCrop::GetSinkPad () {
  return this->GetPad ("sink");
}

GstPad *
VideoCrop::GetSrcPad () {
  return this->GetPad ("src");
}

GstPad *
VideoCrop::GetPad (const std::string &name) {
  GstElement *element = this->GetElement ();
  GstPad *pad = gst_element_get_static_pad (element, name.c_str());

  return pad;
}
