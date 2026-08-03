#include "image_processing.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bmp_image *load_bmp(char *path) {

  bmp_image *bmp = NULL;
  FILE *img_file = fopen(path, "rb");

  if (img_file) {
    bmp = calloc(1, sizeof(bmp_image));
    struct header_bmp *hdr = &bmp->info.header_bmp;
    uint16_t signature;
    size_t r1 = fread(&signature, 1, 2, img_file);

    if (signature != BMP_SIGNATURE) {
      perror("ERRO na assinatura do header.\n");
      fclose(img_file);
      free(bmp);
      return NULL;
    }
    size_t r2 = fread(bmp->info.bytes, 1, TOTAL_BMPH_BYTES, img_file);

    printf("signature:          %X\n", signature);
    printf("file_size:          %u\n", hdr->file_size);
    printf("reserved:           %u\n", hdr->reserved);
    printf("data_offset:        %u\n", hdr->data_offset);
    printf("size:               %u\n", hdr->size_info);
    printf("width:              %u\n", hdr->width);
    printf("height:             %u\n", hdr->height);
    printf("planes:             %u\n", hdr->planes);
    printf("bit_count:          %u\n", hdr->bit_count);
    printf("compression:        %u\n", hdr->compression);
    printf("image_size:         %u\n", hdr->image_size);
    printf("xpixel/M:           %u\n", hdr->xpixel_per_M);
    printf("ypixel/M:           %u\n", hdr->ypixel_per_M);
    printf("colors_used:        %u\n", hdr->colors_used);
    printf("colors_important:   %u\n", hdr->colors_important);

    bmp->n_channels = hdr->width * 3;
    bmp->n_pixels = hdr->width * hdr->height;
    bmp->pw_padded = (bmp->n_channels + 3) & ~3;

    uint32_t image_size = bmp->pw_padded * hdr->height;
    bmp->pixels = malloc(image_size);
    bmp->t_pixels = malloc(sizeof(float) * bmp->n_pixels);
    bmp->ot_pixels = malloc(sizeof(float) * bmp->n_pixels);

    uint32_t pw = bmp->pw_padded;
    uint32_t h = hdr->height;

    if (hdr->data_offset > 54)
      fseek(img_file, hdr->data_offset - 54, SEEK_CUR);
    size_t ret = fread(bmp->pixels, 1, image_size, img_file);
    norm_img_bmp(bmp);

  } else
    perror("ERRO ao encontrar arquivo.\n");

  fclose(img_file);
  return bmp;
}

void create_copy_bmp(bmp_image *bmp, char *name_file) {
  FILE *t_img = fopen(name_file, "wb");

  if (!t_img) {
    perror("ERRO ao criar arquivo.\n");
    return;
  }

  uint32_t pw = bmp->pw_padded;
  uint32_t w_raw = bmp->info.header_bmp.width;
  uint32_t h = bmp->info.header_bmp.height;

  bmp->info.header_bmp.data_offset = 54;
  bmp->info.header_bmp.size_info = 40;
  bmp->info.header_bmp.file_size = 54 + bmp->pw_padded * h;

  for (int i = h - 1, j; i >= 0; i--)
    for (j = 0; j < bmp->n_channels; j += 3) {
      int index = i * pw + j;
      int index_raw = (h - 1 - i) * w_raw + j / 3;
      uint8_t r = bmp->t_pixels[index_raw] * 255.0 + .5,
              g = bmp->t_pixels[index_raw] * 255.0 + .5,
              b = bmp->t_pixels[index_raw] * 255.0 + .5;
      bmp->pixels[index + 2] = r;
      bmp->pixels[index + 1] = g;
      bmp->pixels[index] = b;
    }
  uint16_t sig = BMP_SIGNATURE;
  fwrite(&sig, 2, 1, t_img);
  fwrite(bmp->info.bytes, 1, TOTAL_BMPH_BYTES, t_img);

  fwrite(bmp->pixels, 1, bmp->pw_padded * h, t_img);
  fclose(t_img);
}

void create_bmp(const char *name_file, uint8_t *bytes, uint32_t w, uint32_t h) {
  FILE *img = fopen(name_file, "wb");
  if (!img) {
    perror("erro ao criar imagem.\n");
    exit(1);
  }
  uint16_t sig = BMP_SIGNATURE;
  fwrite(&sig, 2, 1, img);

  uint32_t pw_padded = (w * 3 + 3) & ~3;
  uint32_t f_size = pw_padded * h;
  struct header_bmp *h_bmp = calloc(1, sizeof(struct header_bmp));
  h_bmp->data_offset = 54;
  h_bmp->size_info = 40;
  h_bmp->file_size = 54 + f_size;
  h_bmp->width = w;
  h_bmp->height = h;
  h_bmp->planes = 1;
  h_bmp->bit_count = 24;
  h_bmp->image_size = f_size;

  fwrite(h_bmp, 1, TOTAL_BMPH_BYTES, img);
  fwrite(bytes, 1, f_size, img);
  fclose(img);
  free(h_bmp);
}

void norm_img(image_prcss *img) {
  size_t k = 0;
  float scale = 1.0 / 255;
  uint32_t h = img->height;
  for (int i = h - 1, j; i >= 0; i--)
    for (j = 0; j < img->n_channels; j += 3, k++) {
      int index = i * img->pw_padded + j;
      uint8_t red = img->pixels[index + 2];
      uint8_t green = img->pixels[index + 1];
      uint8_t blue = img->pixels[index];
      float norm = (0.299 * red + 0.587 * green + 0.114 * blue);
      img->hist_n[(uint8_t)(norm + .5)]++;
      img->ot_pixels[k] = norm * scale;
      img->t_pixels[k] = img->ot_pixels[k];
    }
}
void norm_img_bmp(bmp_image *img){
  size_t k = 0;
  float scale = 1.0 / 255;
  uint32_t h = img->info.header_bmp.height;
  for (int i = h - 1, j; i >= 0; i--)
    for (j = 0; j < img->n_channels; j += 3, k++) {
      int index = i * img->pw_padded + j;
      uint8_t red = img->pixels[index + 2];
      uint8_t green = img->pixels[index + 1];
      uint8_t blue = img->pixels[index];
      float norm = (0.299 * red + 0.587 * green + 0.114 * blue);
      img->hist_n[(uint8_t)(norm + .5)]++;
      img->ot_pixels[k] = norm * scale;
      img->t_pixels[k] = img->ot_pixels[k];
    }
}
void norm_img_raw(image_prcss *img)
{
    size_t k = 0;
    uint32_t h = img->height;
    for (int i = 0, j; i < h; i++)
        for (j = 0; j < img->n_channels; j += 3, k++)
        {
            int index = i * img->pw_padded + j;
            uint8_t red = img->pixels[index + 2];
            uint8_t green = img->pixels[index + 1];
            uint8_t blue = img->pixels[index];
            uint8_t norm =
                (uint8_t)round(0.299f * red + 0.587f * green + 0.114f * blue);

            img->t_raw_pixels[index] = norm;
            img->t_raw_pixels[index + 1] = norm;
            img->t_raw_pixels[index + 2] = norm;
        }
}

void set_lut(image_prcss *img, float *lut) { img->lut = lut; }

void reset_transform(image_prcss *img) {
  if (img->lut)
    memset(img->lut, 0, sizeof(float) * 256);
  for (size_t i = 0; i < img->n_pixels; i++)
    img->t_pixels[i] = img->ot_pixels[i];
}

void compute_statistics(image_prcss *img) {
  for (size_t k = 0; k < 256; k++)
    img->pdf_n[k] = img->hist_n[k] / (float)img->n_pixels;
}

void equalize_histogram(image_prcss *img) {
  compute_statistics(img);
  float acc_n = 0;
  for (size_t k = 0; k < 256; k++) {
    acc_n += img->pdf_n[k];
    img->lut[k] = acc_n;
  }
}

void apply_lut(image_prcss *img) {
  for (size_t i = 0; i < img->n_pixels; i++)
    img->t_pixels[i] = img->lut[(uint8_t)(img->t_pixels[i] * 255)];
}

void build_gamma_lut(float *lut, float gamma) {
  for (size_t i = 0; i < 256; i++)
    lut[i] = pow(i / 255.0, gamma);
}

void build_log_lut(float *lut) {
  float c = 1 / log(256);
  for (size_t i = 0; i < 256; i++)
    lut[i] = c * log(1 + i);
}

void contrast_stretch(image_prcss *img) {
  float min = 1, max = 0;
  for (size_t i = 0; i < img->n_pixels; i++)
    min = img->t_pixels[i] < min ? img->t_pixels[i] : min,
    max = img->t_pixels[i] > max ? img->t_pixels[i] : max;
  min *= 255.0;
  max *= 255.0;
  if (max == min)
    return;
  float k = 1.0 / (max - min);

  for (size_t i = 0; i < 256; i++)
    img->lut[i] = i >= min && i <= max ? k * (i - min) : i > min;
}

void apply_otsu_threshold(image_prcss *img) {
  compute_statistics(img);

  float mi_total = 0, wb = 0, sb = 0, max_var = 0, var, wo, mi_b, mi_f;

  uint8_t lim = 0;

  for (size_t i = 0; i < 256; i++)
    mi_total += img->pdf_n[i] * i;
  for (size_t i = 0; i < 256; i++) {
    wb += img->pdf_n[i];
    wo = 1.0 - wb;
    if (wo < 1.e-8)
      break;
    if (wb < 1.e-8)
      continue;
    sb += img->pdf_n[i] * i;
    mi_b = sb / wb;
    mi_f = (mi_total - sb) / wo;
    var = wb * wo * (mi_b - mi_f) * (mi_b - mi_f);

    if (max_var < var)
      lim = i, max_var = var;
  }
  for (size_t i = 0; i < img->n_pixels; i++)
    img->t_pixels[i] = (uint8_t)(img->t_pixels[i] * 255) > lim;
}

void convolution2D(image_prcss *img, float *filter, float *buffer, uint32_t k) {
  uint32_t w = img->width;
  uint32_t h = img->height;

  size_t r = k / 2;
  for (int i = 0, j; i < h; i++)
    for (j = 0; j < w; j++) {
      float value = 0;
      int32_t index_f = 0;
      for (int32_t m = i - r; m <= i + r; m++)
        for (int32_t n = j - r; n <= j + r; n++, index_f++) {          
          int32_t nc = (n < w - 1 ? n : w - 1), mc = (m < h - 1 ? m : h - 1);
          nc *= nc > 0;
          mc *= mc > 0;
          value += img->t_pixels[mc * w + nc] * filter[index_f];          
        }
      buffer[i * w + j] = value;
    }
}

void convolution1D(image_prcss *img, float *k_x, float *k_y, float *buffer,uint32_t k) {
  int32_t w = img->width;
  int32_t h = img->height;
  int32_t r = k / 2;
  float *temp = malloc(sizeof(float) * w * h);

  for (int32_t i = 0, j; i < h; i++)
    for (j = 0; j < w; j++) {
      float value = 0;
      int32_t index_f = 0;
      for (int32_t m = j - r; m <= j + r; m++, index_f++) {

        int32_t mc = m < w ? m : (w - 1);
        mc *= mc >= 0;
        value += img->t_pixels[i * w + mc] * k_x[index_f];
      }
      temp[i * w + j] = value;
    }

  for (int32_t i = 0, j; i < h; i++)
    for (j = 0; j < w; j++) {
      float value = 0;
      int32_t index_f = 0;
      for (int32_t m = i - r; m <= i + r; m++, index_f++) {
        int32_t mc = m < h ? m : (h - 1);
        mc *= mc >= 0;
        value += temp[mc * w + j] * k_y[index_f];
      }
      buffer[i * w + j] = value;
    }

  free(temp);
}

float *new_gauss_filterl2D(uint32_t k, float sig) {
  float *kernel = malloc(sizeof(float) * k * k);
  float k0 = 2.0 * sig * sig, k1 = 1.0 / (3.14159265 * k0);
  float sum = 0;
  int32_t r = k / 2, i = 0;
  for (int32_t y = -r; y <= r; y++)
    for (int32_t x = -r; x <= r; x++, i++) {
      kernel[i] = k1 * expf(-(x * x + y * y) / k0);
      sum += kernel[i];
    }
  for (int i = 0; i < k * k; i++)
    kernel[i] /= sum;
  return kernel;
}

float *new_gauss_filterl1D(uint32_t k, float sig) {
  float *kernel = malloc(sizeof(float) * k);
  float k0 = 2.0f * sig * sig;
  float k1 = 1.0f / sqrtf(2.0f * 3.14159265f * k0);
  int32_t r = k / 2;
  float sum = 0.0f;

  for (int i = 0; i < k; i++) {
    int32_t x = i - r;
    kernel[i] = k1 * expf(-(x * x) / k0);
    sum += kernel[i];
  }

  for (int i = 0; i < k; i++)
    kernel[i] /= sum;
  return kernel;
}

void apply_gaussian_2D(image_prcss *img, uint32_t k, float *buffer, float *filter) {
  convolution2D(img, filter, buffer, k);

  uint32_t w = img->width;
  uint32_t h = img->height;
  uint32_t r = k / 2;

  for (uint32_t i = 0; i < h; i++) {
    for (uint32_t j = 0; j < w; j++) {
      uint32_t idx_buf = i * w + j;
      uint32_t idx_img = i * w + j;
      img->t_pixels[idx_img] = buffer[idx_buf];
    }
  }
}

void apply_sobel_2D(image_prcss *img, float *buffer, float *gx,
                    float *gy) {
  static float sobelx[] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
  static float sobely[] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
  convolution2D(img, sobelx, gx, 3);
  convolution2D(img, sobely, gy, 3);

  uint32_t w = img->width;
  uint32_t h = img->height;
  uint32_t r = 1;  
  uint32_t total = w * h;

  float max = -INFINITY;
  for (uint32_t i = 0; i < total; i++) {
    buffer[i] = sqrtf(gx[i] * gx[i] + gy[i] * gy[i]);
    if (buffer[i] > max)
      max = buffer[i];
  }

  float inv_max = (max < 1e-8f) ? 1.0f : 1.0f / max;

  for (uint32_t i = 0; i < h; i++)
    for (uint32_t j = 0; j < w; j++) {
      uint32_t idx_buf = i * w + j;
      uint32_t idx_img = i * w + j;
      img->t_pixels[idx_img] = inv_max * buffer[idx_buf];
    }
}

void apply_gaussian_1D(image_prcss *img, uint32_t k,
                       float *buffer, float *k_x, float *k_y) {
  convolution1D(img, k_x, k_y, buffer, k);

  uint32_t w = img->width;
  uint32_t h = img->height;

  for (uint32_t i = 0; i < w; i++) {
    for (uint32_t j = 0; j < w; j++) {
      uint32_t idx_buf = i * w + j;
      uint32_t idx_img = (i + 0) * w + (j + 0);
      img->t_pixels[idx_img] = buffer[idx_buf];
    }
  }
}

void apply_sobel_1D(image_prcss *img, float *buffer, float *gx,
                    float *gy) {
  uint32_t k = 3;
  static float deriv_h[]  = {1, 0,-1};
  static float deriv_v[]  = {1, 0,-1};
  static float smooth_v[] = {1, 2, 1};
  static float smooth_h[] = {1, 2, 1};

  convolution1D(img, deriv_h, smooth_v, gx, k);
  convolution1D(img, smooth_h, deriv_v, gy, k);

  uint32_t w = img->width;
  uint32_t h = img->height;
  uint32_t total = w * h;
  uint32_t r = 1;

  float max = -INFINITY;
  for (uint32_t i = 0; i < total; i++) {
    buffer[i] = sqrtf(gx[i] * gx[i] + gy[i] * gy[i]);
    if (buffer[i] > max)
      max = buffer[i];
  }

  float inv_max = (max < 1e-8f) ? 1.0f : 1.0f / max;

  for (uint32_t i = 0; i < h; i++)
    for (uint32_t j = 0; j < w; j++) {
      uint32_t idx_buf = i * w + j;
      uint32_t idx_img = i * w + j;
      img->t_pixels[idx_img] = inv_max * buffer[idx_buf];
    }
}

void apply_canny(image_prcss *img, float *gx, float *gy, float t_high, float t_low) {

  static float deriv_h[] = {1, 0, -1};
  static float smooth_v[] = {1, 2, 1};

  static float smooth_h[] = {1, 2, 1};
  static float deriv_v[] = {1, 0, -1};

  static int32_t dir_x[]={0, 0,1,-1,1, 1,-1,-1};
  static int32_t dir_y[]={1,-1,0, 0,1,-1, 1,-1};
  

  uint32_t w = img->width;
  uint32_t h = img->height;
  uint32_t total = w*h;
  uint32_t r = 1;

  float *mag = malloc(total * sizeof(float));
  float *ang = malloc(total * sizeof(float));

  

  convolution1D(img, deriv_h, smooth_v, gx, 3);
  convolution1D(img, smooth_h, deriv_v, gy, 3);

  float max = -INFINITY;
  float rad_grad = 180.f / 3.14159265f;
  for (uint32_t i = 0; i < total; i++) {
    mag[i] = sqrtf(gx[i] * gx[i] + gy[i] * gy[i]);
    ang[i] = rad_grad * fabsf(atan2f(gy[i], gx[i]));
    if (mag[i] > max)
      max = mag[i];
  }
  t_low *= max;
  t_high *= max;
  memset(img->t_pixels, 0, w * h * sizeof(float));
  for (int i_y = 1; i_y < h-1; i_y++) {
    for (int i_x = 1; i_x < w-1; i_x++) {
      int i = i_y * w + i_x;
      float grad_ang = ang[i], before, after;

      if ((0 <= grad_ang && grad_ang < 22.5) ||
          (157.5 <= grad_ang && grad_ang <= 180)) {
        before = mag[(i_y - 1) * w + i_x + 1];
        after = mag[(i_y + 1) * w + i_x - 1];
      } else if (22.5 <= grad_ang && grad_ang < 67.5) {
        before = mag[(i_y - 1) * w + i_x + 1];
        after = mag[(i_y + 1) * w + i_x - 1];

      } else if (67.5 <= grad_ang && grad_ang < 112.5) {
        before = mag[(i_y - 1) * w + i_x];
        after = mag[(i_y + 1) * w + i_x];
      } else {
        before = mag[(i_y - 1) * w + i_x - 1];
        after  = mag[(i_y + 1) * w + i_x + 1];
      }
      int idx_img = i_y * w + i_x;
      if (mag[i] >= before && mag[i] >= after) {
        float val = mag[i];
        img->t_pixels[idx_img]=val >= t_high?1 :val >= t_low ? .5f : 0.f ;
      } else
        img->t_pixels[idx_img] = 0;
    }
  }
  int32_t *stack_x = malloc(total * sizeof(int32_t));
  int32_t *stack_y = malloc(total * sizeof(int32_t));
  int8_t *visited = calloc(total , sizeof(int8_t));
  int32_t top=0;
  
  for (int i_y = 0; i_y < h; i_y++) 
    for (int i_x = 0; i_x < w; i_x++) {
      int32_t idx=i_y * w + i_x;
      if(img->t_pixels[ idx]<1)continue;
      stack_x[0]=i_x;
      stack_y[0]=i_y;
      visited[idx] = 1;
      top = 1;
      while(top>0){
        top--;
        int32_t curr_x=stack_x[top];
        int32_t curr_y=stack_y[top];
        for (int32_t i = 0; i < 8; i++){
          int32_t tx=curr_x+dir_x[i];
          int32_t ty=curr_y+dir_y[i];
          int32_t neighb=ty*w+tx;          
          if( tx<0 && ty<0 && tx>=w && ty>=h)continue;

          if(!visited[idx] && img->t_pixels[neighb]==.5f){
            visited[idx]=1;
            img->t_pixels[neighb]=1.f;
            stack_x[top]=tx;
            stack_y[top]=ty;
            top++;
          }
        }
      }
    }

  for (uint32_t i = 0; i < total; i++) 
    if (img->t_pixels[i] == 0.5f) 
        img->t_pixels[i] = 0.0f;
    
  free(mag);
  free(ang);
  free(stack_x);
  free(stack_y);
  free(visited);
}

void build_sigmoid_lut(float *lut, float c, float m) {
  float s0 = 1.0 / (1.0 + exp(c * m)), s1 = 1.0 / (1.0 + exp(-c * (1.0 - m)));

  float k = 1.0 / (s1 - s0);
  for (size_t i = 0; i < 256; i++) {
    lut[i] = k * ((1.0 / (1.0 + exp(-c * (i / 255.0 - m)))) - s0);
  }
}

void print_img(image_prcss *img) {
  puts("\x1b[45mdisplay image\x1b[0m\n");
  uint32_t w = img->pw_padded;
  uint32_t h = img->height;

  for (int i = h - 1, j; i >= 0; i--) {
    for (j = 0; j < img->n_channels; j += 3) {
      int index = i * w + j;
      uint8_t red = img->pixels[index + 2];
      uint8_t green = img->pixels[index + 1];
      uint8_t blue = img->pixels[index];
      printf("\x1b[48;2;%u;%u;%um  ", red, green, blue);
    }
    puts("\033[0m");
  }
  puts("\n");
  w = img->width;
  for (int i = 0, j; i < h; i++) {
    for (j = 0; j < w; j++) {
      int index = i * w + j;
      uint32_t gray = img->t_pixels[index] * 255;
      printf("\x1b[48;2;%u;%u;%um  ", gray, gray, gray);
    }
    puts("\033[0m");
  }
}

void destroy_bmp(bmp_image *bmp) {
  free(bmp->pixels);
  free(bmp->t_pixels);
  free(bmp->ot_pixels);
  free(bmp);
}