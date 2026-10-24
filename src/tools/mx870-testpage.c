/*
 * mx870-testpage - diagnostic page for the Canon MX870, A4 plain paper.
 *
 *   mx870-testpage OUT QUALITY CHART LINES...
 *
 * QUALITY = draft | standard | high. Writes one page per LINES value (raster
 * lines per ESC (F block, 0 = automatic). Page n has n black marks top left.
 *
 * CHART "levels": four rows, cyan, magenta, yellow, black, of eight patches at
 * 20, 33, 40, 50, 60, 67, 80 and 100 % ink. These amounts are exact output
 * levels for inks with 2, 3, 4 or 6 levels, so such patches carry no dithering:
 * each shows one drop level printed alone.
 *
 * CHART "rows": two rows, black then cyan, of sixteen patches; patch k inks
 * only raster line k of every 16 (full ink). A patch that stays blank shows a
 * line position the printer drops in each block.
 */

#include "../libmx870/mx870.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int amounts[8] = {51, 85, 102, 127, 153, 170, 204, 255};

static int write_file(void *ctx, const void *buf, size_t len)
{
  return fwrite(buf, 1, len, ctx) == len ? 0 : -1;
}

/* White line, with the page number marks (1/600 inch layout) where y crosses them. */
static void page_marks(uint8_t *rgb, int w, int dpi, int y, int page)
{
  memset(rgb, 255, (size_t)w * 3);
  if (y >= 60 && y < 180)
    for (int i = 0; i < page; i++)
      for (int x = (60 + i * 180) * dpi / 600; x < (180 + i * 180) * dpi / 600; x++)
        memset(rgb + 3 * x, 0, 3);
}

/* Line y of the chart: 4 rows of 1 cm, 5 mm apart, from 15 mm down (layout in 1/600 inch). */
static void chart_line(uint8_t *rgb, int w, int dpi, int y, int page)
{
  int row = (y - 354) / 354, in_row = (y - 354) % 354;

  page_marks(rgb, w, dpi, y, page);
  if (y < 354 || row > 3 || in_row >= 236)
    return;
  for (int x = 0; x < w; x++) {
    int x600 = x * 600 / dpi, col = x600 / 600, v;

    if (col > 7 || x600 % 600 >= 560)
      continue;
    v = amounts[col];
    if (row == 3) {
      /* Neutral input: black ink only. */
      memset(rgb + 3 * x, 255 - v, 3);
    } else {
      /* One primary: that ink only, no black generation. */
      rgb[3 * x + row] = (uint8_t)(255 - v);
    }
  }
}

/* Line y (raster lines at dpi) of the rows chart, same frame as the levels chart. */
static void rows_line(uint8_t *rgb, int w, int dpi, int y, int page)
{
  int y600 = y * 600 / dpi, row = (y600 - 354) / 354, in_row = (y600 - 354) % 354;

  page_marks(rgb, w, dpi, y600, page);
  if (y600 < 354 || row > 1 || in_row >= 236)
    return;
  for (int x = 0; x < w; x++) {
    int x600 = x * 600 / dpi, k = x600 / 300;

    if (k > 15 || x600 % 300 >= 260 || y % 16 != k)
      continue;
    if (row == 0)
      memset(rgb + 3 * x, 0, 3);
    else
      rgb[3 * x] = 0;
  }
}

int main(int argc, char **argv)
{
  mx870_page_settings_t s = {0};
  mx870_job_t *job;
  FILE *f;
  int rc = 0;

  int rows;

  if (argc < 5 || (strcmp(argv[3], "levels") && strcmp(argv[3], "rows"))) {
    fprintf(stderr, "usage: mx870-testpage OUT draft|standard|high levels|rows LINES...\n");
    return 2;
  }
  rows = !strcmp(argv[3], "rows");
  s.paper = mx870_find_paper("iso_a4_210x297mm");
  s.media = mx870_find_media("stationery");
  s.quality = !strcmp(argv[2], "draft") ? MX870_QUALITY_DRAFT
            : !strcmp(argv[2], "standard") ? MX870_QUALITY_STANDARD : MX870_QUALITY_HIGH;
  s.color = MX870_COLOR_RGB;
  if (!(f = fopen(argv[1], "wb"))) {
    perror(argv[1]);
    return 1;
  }
  job = mx870_job_new(write_file, f);

  for (int p = 4; p < argc && !rc; p++) {
    int w, h, dpi;
    uint8_t *line;

    mx870_job_set_lines_per_block(job, atoi(argv[p]));
    if (mx870_page_start(job, &s)) {
      rc = -1;
      break;
    }
    mx870_page_geometry(job, &w, &h, &dpi, NULL, NULL);
    line = malloc((size_t)w * 3);
    for (int y = 0; y < h && !rc; y++) {
      if (rows)
        rows_line(line, w, dpi, y, p - 3);
      else
        chart_line(line, w, dpi, y * 600 / dpi, p - 3);
      rc = mx870_page_write_line(job, line);
    }
    free(line);
    if (!rc)
      rc = mx870_page_end(job);
  }
  if (!rc)
    rc = mx870_job_end(job);
  mx870_job_free(job);
  if (fclose(f))
    rc = -1;
  if (rc)
    fprintf(stderr, "mx870-testpage: encoding failed\n");
  return rc ? 1 : 0;
}
