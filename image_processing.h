#include <stdint.h>
#ifndef IMG_PRCSS
#define IMG_PRCSS

#define TOTAL_BMPH_BYTES 52
#define BMP_SIGNATURE 0x4D42

typedef struct image_prcss {
  uint8_t *pixels, *t_raw_pixels;
  uint32_t pw_padded, n_channels, n_pixels, hist_n[256];
  uint32_t width;
  uint32_t height;
  float *t_pixels, *ot_pixels, pdf_n[256], *lut;

} image_prcss;

typedef struct bmp_image {
  union info {
    struct header_bmp {
      // file header
      uint32_t file_size;
      uint32_t reserved;
      uint32_t data_offset;
      // info header
      uint32_t size_info;
      uint32_t width;
      uint32_t height;
      uint16_t planes;
      uint16_t bit_count;
      uint32_t compression;
      uint32_t image_size;
      uint32_t xpixel_per_M;
      uint32_t ypixel_per_M;
      uint32_t colors_used;
      uint32_t colors_important;
    } header_bmp;
    uint8_t bytes[TOTAL_BMPH_BYTES];
  } info;
  uint8_t *pixels, *t_raw_pixels;

  uint32_t pw_padded, n_channels, n_pixels, hist_n[256];

  float *t_pixels, *ot_pixels, pdf_n[256], *lut;
} bmp_image;

bmp_image *load_bmp(char *path);
void create_copy_bmp(bmp_image *img, char *name_file);
void norm_img(bmp_image *bmp);
void norm_img_raw(bmp_image *bmp);
void create_bmp(const char *name_file, uint8_t *bytes, uint32_t w, uint32_t h);
float *create_buffer(uint32_t iw, uint32_t ih, uint32_t fsize);

void set_lut(bmp_image *bmp, float *lut);
void apply_lut(bmp_image *bmp);
void reset_transform(bmp_image *bmp);
void compute_statistics(bmp_image *bmp);
void equalize_histogram(bmp_image *bmp);

void build_gamma_lut(float *lut, float gamma);
void build_log_lut(float *lut);
void build_sigmoid_lut(float *lut, float c, float m);
void contrast_stretch(bmp_image *bmp);
void apply_otsu_threshold(bmp_image *bmp);

void convolution2D(bmp_image *bmp, float *filter, float *buffer, uint32_t k);

void convolution1D(bmp_image *bmp, float *k_x, float *k_y, float *buffer,
                   uint32_t k);

void apply_sobel_2D(bmp_image *bmp, float *buffer, float *gx,
                    float *gy);

void apply_sobel_1D(bmp_image *bmp, float *buffer, float *gx,
                    float *gy);

void apply_gaussian_2D(bmp_image *bmp, uint32_t k,
                       float *buffer, float *filter);

void apply_gaussian_1D(bmp_image *bmp, uint32_t k,
                       float *buffer, float *k_x, float *k_y);

void apply_canny(bmp_image *bmp, float *gx,float *gy, float t_high, float t_low);
            
float *new_gauss_filterl2D(uint32_t k, float sig);
float *new_gauss_filterl1D(uint32_t k, float sig);
void print_bmp(bmp_image *bmp);
void destroy_bmp(bmp_image *bmp);

#endif