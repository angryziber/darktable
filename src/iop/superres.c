/*
    This file is part of darktable,
    copyright (c) 2012 johannes hanika.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
// our includes go first:
#include "develop/imageop.h"
#include "bauhaus/bauhaus.h"
#include "gui/gtk.h"
#include "gui/simple_gui.h"

#include <gtk/gtk.h>
#include <stdlib.h>

DT_MODULE(1)

typedef struct dt_iop_useless_global_data_t
{
}
dt_iop_useless_global_data_t;

// this returns a translatable name
const char *name()
{
  return _("super resolution");
}

// some additional flags (self explanatory i think):
int
flags()
{
  return IOP_FLAGS_INCLUDE_IN_STYLES;
}

// where does it appear in the gui?
int
groups()
{
  return IOP_GROUP_CORRECT;
}

// TODO: we want to upsize by 2x--3x!

/** modify regions of interest (optional, per pixel ops don't need this) */
// void modify_roi_out(struct dt_iop_module_t *self, struct dt_dev_pixelpipe_iop_t *piece, dt_iop_roi_t *roi_out, const dt_iop_roi_t *roi_in);
// void modify_roi_in(struct dt_iop_module_t *self, struct dt_dev_pixelpipe_iop_t *piece, const dt_iop_roi_t *roi_out, dt_iop_roi_t *roi_in);

/** process, all real work is done here. */
void process(struct dt_iop_module_t *self, dt_dev_pixelpipe_iop_t *piece, void *i, void *o, const dt_iop_roi_t *roi_in, const dt_iop_roi_t *roi_out)
{
  float *d = (float *)piece->data; // the default param format is an array of int or float, depending on the type of widget
  const float radius   = d[0];
  const float strength = d[1];
  const float scale    = d[2];
  const float luma     = d[3];
  const float chroma   = d[4];

  // adjust to zoom size:
  const int P = ceilf(radius * roi_in->scale / piece->iscale); // pixel filter size
  const int K = ceilf(7 * roi_in->scale / piece->iscale); // nbhood
  const float sharpness = 100000.0f/(1.0f+strength);
  if(P < 1)
  {
    // nothing to do from this distance:
    memcpy (ovoid, ivoid, sizeof(float)*4*roi_out->width*roi_out->height);
    return;
  }

  // adjust to Lab, make L more important
  // TODO: put that where?
  // float max_L = 120.0f, max_C = 512.0f;
  // float nL = 1.0f/max_L, nC = 1.0f/max_C;
  // const float norm2[4] = { nL*nL, nC*nC, nC*nC, 1.0f };

  float *tmp = dt_alloc_align(64, sizeof(float)*roi_out->width*dt_get_num_threads());
  // we want to sum up weights in col[3], so need to init to 0:
  memset(ovoid, 0x0, sizeof(float)*roi_out->width*roi_out->height*4);

  // TODO: 
  
  dt_nlm_accum_scaled(input_features, prior_payload, prior_features, accum, width, height, prior_width, prior_height, P, K, sharpness, tmp);
  // TODO: multiple scales for denoising?
  dt_nlm_normalize(input_payload, accum, width, height, luma, chroma);

  free(tmp);

  if(piece->pipe->mask_display)
    dt_iop_alpha_copy(ivoid, ovoid, roi_out->width, roi_out->height);
}


/** init, cleanup, commit to pipeline. when using the simple api you don't need to care about params, ... */
void init(dt_iop_module_t *module)
{
  // order has to be changed by editing the dependencies in tools/iop_dependencies.py
  module->priority = 471; // module order created by iop_dependencies.py, do not edit!
}

/** gui callbacks, these are needed. */
dt_gui_simple_t* gui_init_simple(dt_iop_module_so_t *self) // sorry, only dt_iop_module_so_t* in here :(
{
  static dt_gui_simple_t gui =
  {
    0, // not used currently
    {
      /* patch size */
      {
        .slider = {
          DT_SIMPLE_GUI_SLIDER,
          "patch size",                                     // always make sure to add an id
          N_("patch size"),                                 // just mark the strings for translation using N_()
          N_("radius of the patches to search for"),        // same here
          NULL,                                             // the rest are specific settings for sliders
          1.0f, 4.0f, 1.0f, 2.0f,
          0,
          NULL,                                        // when no callback is specified a default one is used
          NULL                                         // no parameter means self. keep that in mind when you want to pass the number 0!
        }
      },

      /* strength */
      {
        .slider = {
          DT_SIMPLE_GUI_SLIDER,
          "strength",                                     // always make sure to add an id
          N_("strength"),                                 // just mark the strings for translation using N_()
          N_("strength of the effect"),                   // same here
          NULL,                                           // the rest are specific settings for sliders
          0.0f, 100.0f, 1.0f, 50.0f,
          0,
          NULL,                                           // when no callback is specified a default one is used
          NULL                                            // no parameter means self. keep that in mind when you want to pass the number 0!
        }
      },

      /* scale */
      {
        .slider = {
          DT_SIMPLE_GUI_SLIDER,
          "scale",                                     // always make sure to add an id
          N_("scale"),                                 // just mark the strings for translation using N_()
          N_("how much to scale up"),                  // same here
          NULL,                                        // the rest are specific settings for sliders
          1.0f, 3.0f, 0.1f, 2.0f,
          0,
          NULL,                                        // when no callback is specified a default one is used
          NULL                                         // no parameter means self. keep that in mind when you want to pass the number 0!
        }
      },

      /* luma */
      {
        .slider = {
          DT_SIMPLE_GUI_SLIDER,
          "luma",                                     // always make sure to add an id
          N_("luma"),                                 // just mark the strings for translation using N_()
          N_("how much to affect brightness"),        // same here
          NULL,                                       // the rest are specific settings for sliders
          0.0f, 100.0f, 1.0f, 50.0f,
          0,
          NULL,                                       // when no callback is specified a default one is used
          NULL                                        // no parameter means self. keep that in mind when you want to pass the number 0!
        }
      },

      /* chroma */
      {
        .slider = {
          DT_SIMPLE_GUI_SLIDER,
          "chroma",                                     // always make sure to add an id
          N_("chroma"),                                 // just mark the strings for translation using N_()
          N_("how much to affect colors"),              // same here
          NULL,                                         // the rest are specific settings for sliders
          0.0f, 100.0f, 1.0f, 100.0f,
          0,
          NULL,                                         // when no callback is specified a default one is used
          NULL                                          // no parameter means self. keep that in mind when you want to pass the number 0!
        }
      },

      /** the last element has to be of type DT_SIMPLE_GUI_NONE */
      {.common = {DT_SIMPLE_GUI_NONE, NULL, NULL, NULL}}
    }
  };

  return &gui;
}

/** not needed when using the simple gui api. */
// void gui_init(dt_iop_module_t* self);
// void gui_cleanup(dt_iop_module_t *self);
// void gui_update(dt_iop_module_t *self);

/** additional, optional callbacks to capture darkroom center events. */
// void gui_post_expose(dt_iop_module_t *self, cairo_t *cr, int32_t width, int32_t height, int32_t pointerx, int32_t pointery);
// int mouse_moved(dt_iop_module_t *self, double x, double y, int which);
// int button_pressed(dt_iop_module_t *self, double x, double y, int which, int type, uint32_t state);
// int button_released(struct dt_iop_module_t *self, double x, double y, int which, uint32_t state);
// int scrolled(dt_iop_module_t *self, double x, double y, int up, uint32_t state);

// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;