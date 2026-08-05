#include "image_processing.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <math.h>
#include <stdio.h>

int main() {

  bool run = true;

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_CAMERA)) {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Window *window =
      SDL_CreateWindow("window", 640, 480, SDL_WINDOW_RESIZABLE);
  if (!window) {
    printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_Event event;

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);


  SDL_Texture *texture = NULL;
  int32_t device_count = 0;
  SDL_CameraID *devices = SDL_GetCameras(&device_count);
  if (devices == NULL) {
    SDL_Log("Erro ao enumerar cameras: %s", SDL_GetError());
    return 1;
  } else if (device_count == 0) {
    SDL_Log("Erro ao encontrar camera.");
    SDL_free(devices);
    return 1;
  }
  uint32_t width = 640, height = 480;
  uint32_t stride = (width * 3 + 3) & ~3; // stride em bytes (ex: 1920)
  uint32_t image_size = stride * height;
  uint32_t n_pixels = width * height;

  SDL_CameraSpec spec = {.width = width,
                         .height = height,
                         .framerate_numerator = 25,
                         .framerate_denominator = 1,
                         .format = SDL_PIXELFORMAT_BGR24};
  SDL_Camera *camera = SDL_OpenCamera(devices[0], &spec);
  if (camera == NULL) {
    SDL_Log("Erro ao abrir camera: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  uint64_t timestamp = 0;
  SDL_Surface *frame = SDL_AcquireCameraFrame(camera, &timestamp);

  float *buffer = calloc(sizeof(float),n_pixels);
  float *gx = calloc(sizeof(float),n_pixels);
  float *gy = calloc(sizeof(float),n_pixels);
  uint32_t k_gauss = 5;
  float *gauss_f = new_gauss_filterl1D(k_gauss, 1.5f);
  

  image_prcss img = {.n_channels = width * 3,
                   .n_pixels = n_pixels,
                   .pw_padded = stride,
                   .pixels = malloc(image_size),
                   .t_pixels = malloc(sizeof(float) * n_pixels),
                   .ot_pixels = malloc(sizeof(float) * n_pixels),
                   .t_raw_pixels = calloc(image_size, 1),
                   .lut = calloc(256, sizeof(float))};

  img.width = width;
  img.height = height;
  int state = 3;
  while (run) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_KEY_DOWN) {
        state = (state + 1) % 4;
      }

      if (event.type == SDL_EVENT_QUIT)
        run = false;
    }

    uint64_t tm = 0;
    SDL_Surface *frame = SDL_AcquireCameraFrame(camera, &tm);

    if (frame) {
      if (!texture) {
        SDL_SetWindowSize(window, frame->w, frame->h);
        SDL_SetRenderLogicalPresentation(renderer, frame->w, frame->h,
                                         SDL_LOGICAL_PRESENTATION_LETTERBOX);
        texture =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24,
                              SDL_TEXTUREACCESS_STREAMING, frame->w, frame->h);

        frame->pixels;
      }
      if (texture) {

        img.pixels = frame->pixels;
        
        memset(img.lut, 0, sizeof(float) * 256);
        memset(img.pdf_n, 0, sizeof(float) * 256);
        memset(img.hist_n, 0, sizeof(float) * 256);
        norm_img(&img);
        equalize_histogram(&img);
        contrast_stretch(&img);
        apply_lut(&img);
        
        if (state == 0){
          apply_gaussian_1D(&img, k_gauss, buffer, gauss_f, gauss_f);
        }
        if (state == 1) {
          apply_gaussian_1D(&img, k_gauss, buffer, gauss_f,gauss_f);
          apply_sobel_1D(&img, buffer, gx, gy);
        }

        if (state == 2) {
          apply_gaussian_1D(&img, k_gauss, buffer, gauss_f, gauss_f);
          apply_canny(&img, gx, gy, .1, .01);
        }
        uint32_t h = img.height, k = 0;
        for (int i = 0, j; i < h; i++)
          for (j = 0, k = 0; j < img.n_channels; j += 3, k++) {
            int index = i * img.pw_padded + j;
            int index_t = (h - 1 - i) * width + k;
            uint8_t p_value = img.t_pixels[index_t] * 255;
            img.t_raw_pixels[index] = p_value;
            img.t_raw_pixels[index + 1] = p_value;
            img.t_raw_pixels[index + 2] = p_value;
          }
        if (state < 3)
          SDL_UpdateTexture(texture, NULL, img.t_raw_pixels, frame->pitch);
        else
          SDL_UpdateTexture(texture, NULL, frame->pixels, frame->pitch);
      }
      SDL_ReleaseCameraFrame(camera, frame);
    }

    if (texture)
      SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_Delay(32);
  }

  SDL_CloseCamera(camera);
  SDL_free(devices);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}