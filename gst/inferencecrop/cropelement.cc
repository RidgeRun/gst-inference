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

#include "cropelement.h"

CropElement::CropElement ()
{
  this->element = nullptr;
  this->image_width = 0;
  this->image_height = 0;
  this->x = 0;
  this->y = 0;
  this->width = 0;
  this->height = 0;
  this->width_ratio = 1;
  this->height_ratio = 1;
}

bool
CropElement::Validate ()
{
  GstElement * element;
  
  this->mutex.lock ();
  if (nullptr == this->element) {
    const std::string factory = this->GetFactory ();
    this->element = gst_element_factory_make (factory.c_str (), nullptr);
  }
  element = this->element;
  this->mutex.unlock ();
  
  return element != nullptr;
}

GstElement *
CropElement::GetElement ()
{
  GstElement * element;

  this->mutex.lock ();
  element = this->element;
  this->mutex.unlock ();
  
  return element;
}

void
CropElement::Reset ()
{
  this->SetBoundingBox(0, 0, this->image_width, this->image_height,1,1);
}

void
CropElement::SetImageSize (gint width, gint height)
{
  this->mutex.lock ();
  this->image_width = width;
  this->image_height = height;
  this->mutex.unlock ();

  this->Reset ();
}

void
CropElement::SetBoundingBox (gint x, gint y, gint width, gint height, gint width_ratio, gint height_ratio)
{
  this->mutex.lock ();
  this->x = x;
  this->y = y;
  this->width = width;
  this->height = height;
  this->width_ratio = width_ratio;
  this->height_ratio = height_ratio;

  this->UpdateElement (this->element,
		       this->image_width,
		       this->image_height,
		       this->x,
		       this->y,
		       this->width,
		       this->height,
           this->width_ratio,
           this->height_ratio);
  this->mutex.unlock ();
}

CropElement::~CropElement ()
{
  if (nullptr != this->element) {
    gst_object_unref (this->element);
    this->element = nullptr;
  }
}
