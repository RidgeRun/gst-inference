/*
 * GStreamer
 * Copyright (C) 2019 RidgeRun
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

#ifndef __VIDEO_CROP_H__
#define __VIDEO_CROP_H__

#include "cropelement.h"

#include <string>

class VideoCrop : public CropElement
{
protected:
  const std::string GetFactory () const override;
  void UpdateElement (GstElement * element,
		      gint image_width,
		      gint image_height,
		      gint x,
		      gint y,
		      gint width,
		      gint height) override;
private:
  const std::string factory = "videocrop";
};

#endif //__VIDEO_CROP_H__
