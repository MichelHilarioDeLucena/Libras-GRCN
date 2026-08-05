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
void create_bmp(const char *name_file, uint8_t *bytes, uint32_t w, uint32_t h);

void norm_img(image_prcss *img);
void norm_img_bmp(bmp_image *img);
void norm_img_raw(image_prcss *img);

void set_lut(image_prcss *img, float *lut);
void apply_lut(image_prcss *img);
void reset_transform(image_prcss *img);
void compute_statistics(image_prcss *img);
void equalize_histogram(image_prcss *img);

void build_gamma_lut(float *lut, float gamma);
void build_log_lut(float *lut);
void build_sigmoid_lut(float *lut, float c, float m);
void contrast_stretch(image_prcss *img);
void apply_otsu_threshold(image_prcss *img);

void convolution2D(image_prcss *img, float *filter, float *buffer, uint32_t k);

void convolution1D(image_prcss *img, float *k_x, float *k_y, float *buffer,
                   uint32_t k);

void apply_sobel_2D(image_prcss *img, float *buffer, float *gx,
                    float *gy);

void apply_sobel_1D(image_prcss *img, float *buffer, float *gx,
                    float *gy);

void apply_gaussian_2D(image_prcss *img, uint32_t k,
                       float *buffer, float *filter);

void apply_gaussian_1D(image_prcss *img, uint32_t k,
                       float *buffer, float *k_x, float *k_y);

void apply_canny(image_prcss *img, float *gx,float *gy, float t_high, float t_low);
            
float *new_gauss_filterl2D(uint32_t k, float sig);
float *new_gauss_filterl1D(uint32_t k, float sig);
void print_img(image_prcss *img);
void destroy_img(image_prcss *img);
void destroy_img_prcss(image_prcss *img);

#endif