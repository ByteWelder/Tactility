/**
 * @file save_as_bmp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "save_as_bmp.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*BMP*/
typedef struct tagBITMAPFILEHEADER {
  uint16_t   bfType;
  uint32_t   bfSize;
  uint16_t   bfReserved1;
  uint16_t   bfReserved2;
  uint32_t   bfOffBits;
} __attribute__ ((packed)) BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
  uint32_t   biSize;
  uint32_t   biwidth;
  uint32_t   biheight;
  uint16_t   biPlanes;
  uint16_t   biBitCount;
  uint32_t   biCompression;
  uint32_t   biSizeImage;
  uint32_t   biXPelsPerMeter;
  uint32_t   biYPelsPerMeter;
  uint32_t   biClrUsed;
  uint32_t   biClrImportant;
} __attribute__ ((packed)) BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
  uint8_t    rgbBlue;
  uint8_t    rgbGreen;
  uint8_t    rgbRed;
  uint8_t    rgbReserved;
} __attribute__ ((packed)) RGBQUAD;


/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool save_as_bmp_file(uint8_t * image, uint32_t w, uint32_t h, uint32_t bpp, char *filename)
{
    BITMAPFILEHEADER tBmpFileHead;
    BITMAPINFOHEADER tBmpInfoHead;

    uint32_t dwSize;

    uint8_t *pPos = 0;

    uint32_t bw;
	lv_fs_file_t f;

    memset(&tBmpFileHead, 0, sizeof(BITMAPFILEHEADER));
    memset(&tBmpInfoHead, 0, sizeof(BITMAPINFOHEADER));

    lv_fs_res_t res = lv_fs_open(&f, filename, LV_FS_MODE_WR);
    if(res != LV_FS_RES_OK)
    {
      LV_LOG_USER("Can't create output file %s", filename);
      return false;
    }

    tBmpFileHead.bfType     = 0x4d42;
    tBmpFileHead.bfSize     = 0x36 + w * h * (bpp / 8);
    tBmpFileHead.bfOffBits  = 0x00000036;

    tBmpInfoHead.biSize     = 0x00000028;
    tBmpInfoHead.biwidth    = w;
    tBmpInfoHead.biheight   = h;
    tBmpInfoHead.biPlanes   = 0x0001;
    tBmpInfoHead.biBitCount = bpp;
    tBmpInfoHead.biCompression  = 0;
    tBmpInfoHead.biSizeImage    = w * h * (bpp / 8);
    tBmpInfoHead.biXPelsPerMeter    = 0;
    tBmpInfoHead.biYPelsPerMeter    = 0;
    tBmpInfoHead.biClrUsed  = 0;
    tBmpInfoHead.biClrImportant     = 0;

    res = lv_fs_write(&f, &tBmpFileHead, sizeof(tBmpFileHead), &bw);
    if (bw != sizeof(tBmpFileHead))
    {
        LV_LOG_USER("Can't write BMP File Head to %s", filename);
        return false;
    }

    res = lv_fs_write(&f, &tBmpInfoHead, sizeof(tBmpInfoHead), &bw);
    if (bw != sizeof(tBmpInfoHead))
    {
        LV_LOG_USER("Can't write BMP File Info Head to %s", filename);
        return false;
    }

    dwSize = w * bpp / 8;
    pPos   = image + (h - 1) * dwSize;

    while (pPos >= image)
    {
        res = lv_fs_write(&f, pPos, dwSize, &bw);
        if (bw != dwSize)
        {
            LV_LOG_USER("Can't write date to BMP File %s", filename);
            return false;
        }
        pPos -= dwSize;
    }

    lv_fs_close(&f);

    return true;
}

/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/
