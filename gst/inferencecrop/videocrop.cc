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
                        gint top,
                        gint bottom,
                        gint right,
                        gint left) {

  g_return_if_fail (element);

  g_object_set (element,
                "top", top,
                "bottom", bottom,
                "left", left,
                "right", right,
                "qos",FALSE, NULL);
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
