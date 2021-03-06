#ifndef _gl_render_h_
#define _gl_render_h_
#include <d3d9.h>
#include "glad.h"
void video_deinit();
bool video_set_pixel_format(unsigned format);
void video_refresh(const void *data, unsigned width, unsigned height, unsigned pitch);
void video_init(const struct retro_game_geometry *geom, HWND hwnd);

typedef struct {
  IDirect3D9 *d3d;
  IDirect3DDevice9 *d3ddev;
  IDirect3DTexture9 *tex;
  D3DPRESENT_PARAMETERS d3dpp;

  GLuint tex_id;
  GLuint fbo_id;
  GLuint rbo_id;

  int glmajor;
  int glminor;
  int last_h;
  int last_w;

  bool software_rast;
  GLuint pitch;
  bool sw_core;
  GLint tex_w, tex_h;
  GLuint base_w, base_h;
  float aspect;

  GLuint pixfmt;
  GLuint pixtype;
  GLuint bpp;
  HDC   hDC;
  HGLRC hRC;
  HWND hwnd;
  struct retro_hw_render_callback hw;
}video;
extern video g_video;

#endif