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

#include "cropelement.h"

CropElement::CropElement () {
  this->element = nullptr;
  this->top = 0;
  this->bottom = 0;
  this->right = 0;
  this->left = 0;
}

bool
CropElement::Validate () {
  GstElement *element;

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
CropElement::GetElement () {
  GstElement *element;
  element = nullptr;
  bool validated = Validate();
  this->mutex.lock ();
  if (validated) {
    element = this->element;
  }else{
    const std::string factory = this->GetFactory ();
    GST_ERROR_OBJECT (this->element, "Unable to initialize the element %s", factory.c_str ());
  }

  this->mutex.unlock ();

  return element;
}

void
CropElement::Reset () {
  this->SetCroppingSize(0, 0, 0, 0);
}

void
CropElement::SetCroppingSize (gint top, gint bottom, gint right, gint left) {
  this->mutex.lock ();
  this->top = top;
  this->bottom = bottom;
  this->right = right;
  this->left = left;

  this->UpdateElement (this->element,
                       this->top,
                       this->bottom,
                       this->right,
                       this->left);
  this->mutex.unlock ();
}

CropElement::~CropElement () {
  if (nullptr != this->element) {
    gst_object_unref (this->element);
    this->element = nullptr;
  }
}
