#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 1

#include <FS.h>
#include <FastLED.h>

#define baud 115200

const uint8_t width = 16;
const uint8_t height = 16;

const uint16_t numleds = width * height;

// Wemos
#define LED_PIN D3

CRGB leds[numleds];

#if 1
// 8x8
const uint8_t data_invaderApp_4_8x8_bmp[] = {
0x42,0x4d,0x96,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x76,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x08,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,
0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x47,0xff,0x00,0xff,0xff,
0xff,0x00,0x00,0xb9,0xfa,0x00,0x00,0x6a,0xfa,0x00,0x1e,0xd6,0xff,0x00,0x00,0x8a,
0xff,0x00,0x00,0xeb,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x10,0x10,0x10,0x04,0x04,
0x04,0x04,0x00,0x66,0x66,0x60,0x03,0x20,0x32,0x03,0x00,0x55,0x55,0x50,0x00,0x70,
0x07,0x00,0x00,0x00,0x00,0x00};

const unsigned char data_invader_4_8x8_bmp[] = {
0x42,0x4d,0x96,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x76,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x08,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,
0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00,0xff,0xff,
0xff,0x00,0xff,0x00,0x28,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x10,0x10,0x10,0x01,0x01,
0x01,0x01,0x00,0x11,0x11,0x10,0x01,0x32,0x13,0x21,0x00,0x11,0x11,0x10,0x00,0x01,
0x01,0x00,0x00,0x00,0x00,0x00};
#endif

static inline int16_t read_int16(const uint8_t *data, unsigned int offset) {
        return (int16_t) (data[offset] | (data[offset + 1] << 8));
}

static inline uint16_t read_uint16(const uint8_t *data, unsigned int offset) {
        return (uint16_t) (data[offset] | (data[offset + 1] << 8));
}

static inline int32_t read_int32(const uint8_t *data, unsigned int offset) {
        return (int32_t) (data[offset] | (data[offset + 1] << 8) | 
                        (data[offset + 2] << 16) | (data[offset + 3] << 24));
}

static inline uint32_t read_uint32(const uint8_t *data, uint32_t offset) {
        return (uint32_t) (data[offset] | (data[offset + 1] << 8) | 
                          (data[offset + 2] << 16) | (data[offset + 3] << 24));
}

// normal
#if 0
uint16_t XY(uint8_t x, uint8_t y) {
  return y * width + x;
}
#else
// alternate (zigzag)
uint16_t XY(uint8_t x, uint8_t y) {
  uint16_t i;
  if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (width - 1) - x;
      i = (y * width) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * (width)) + x;
    }
    return i;
}
#endif

#define BMP_FILE_HEADER_SIZE 14

typedef enum {
        BMP_ENCODING_RGB = 0,
        BMP_ENCODING_RLE8 = 1,
        BMP_ENCODING_RLE4 = 2,
        BMP_ENCODING_BITFIELDS = 3
} bmp_encoding;

typedef enum {
  RLE_EOL = 0,
  RLE_EOB = 1,
  RLE_DELTA = 2
} rle8;

struct BMPImage {
  uint16_t width;
  uint16_t height;
  CRGB*    data;
};

struct BMPImage* allocateBMP(uint16_t width, uint16_t height) {
  struct BMPImage* image = (struct BMPImage*)malloc(sizeof(struct BMPImage));
  image->width = width;
  image->height = height;
  image->data = (CRGB*)malloc(width * height * sizeof(CRGB));
  return image;
}

void freeBMP(struct BMPImage* image) {
  free(image->data);
  free(image);
}

void setBMPColor(struct BMPImage* image, int16_t x, int16_t y, CRGB color) {
    int idx = y * image->width + x;
    image->data[idx] = color;
}


void drawPixel(int16_t x, int16_t y, CRGB color) {
  if ((x < width) && (y < height))
    leds[XY(x, y)] = color;
}

void displayBMP(struct BMPImage* image, int16_t x, int16_t y) {
  for (int16_t j = 0; j < image->height; ++j) {
    for (int16_t i = 0; i < image->width; ++i) {
      uint16_t idx = j * image->width + i;
      drawPixel(x + i, y + j, image->data[idx]);
    }
  }
}

void displayBMP2(struct BMPImage* image, int16_t x, int16_t y, int16_t z) {
  for (int16_t j = 0; j < image->height; ++j) {
    for (int16_t i = 0; i < image->width; ++i) {
      uint16_t idx = (j+z) * image->width + i;
      drawPixel(x + i, y + j, image->data[idx]);
    }
  }
}


// decodes BMPs in memory 
BMPImage* decodeBMPInMemory(const uint8_t* data) {
  // crashes if data smaller than 2
  if (data[0] != 'B' || data[1] != 'M') 
    return NULL;
    Serial.println("Starting reading from memory");
  uint32_t offset = read_uint32(data, 10);
  Serial.print("offset "); Serial.println(offset);
  const uint8_t* infoheader = data + BMP_FILE_HEADER_SIZE;
  // must be at least 40 for BIMAPINFOHEADER
  uint32_t biSize = read_uint32(infoheader, 0);
  Serial.print("biSize "); Serial.println(biSize);

   int32_t biWidth = read_int32(infoheader, 4);
    Serial.print("biWidth "); Serial.println(biWidth);
     // negative means top down instead of bottom up
    int32_t biHeight = read_int32(infoheader, 8);
    Serial.print("biHeight "); Serial.println(biHeight);
    boolean reversed = false;
    if (biHeight < 0) {
      reversed = true;
      biHeight = -biHeight;
    }
    // allocate BMPImage
    struct BMPImage* image = allocateBMP(biWidth, biHeight);

    uint16_t biBitCount = read_uint16(infoheader, 14);
    Serial.print("biBitCount "); Serial.println(biBitCount);
   int32_t biCompression = read_int32(infoheader, 16);
    Serial.print("biCompression "); Serial.println(biCompression);
    uint32_t bytesPerRow = ((biWidth * biBitCount + 31) / 32) * 4;
    Serial.print("bytesPerRow "); Serial.println(bytesPerRow);
    const uint8_t* pixels = 0;
    const uint8_t* palette = 0;
    if ((biBitCount == 8) || (biBitCount == 4)) { 
      // palette
      palette = infoheader + biSize;
      uint16_t paletteSize = (uint16_t)(1 << biBitCount); // 8 supported
      Serial.print("Palette size "); Serial.println(paletteSize);
       // display palette 
        char buffer[32];     
        for (uint16_t idx = 0; idx < paletteSize; ++idx) {
           sprintf(buffer, "entry %d : %d %d %d", idx, palette[4 * idx + 2], palette[4 * idx + 1], palette[4 * idx + 0]);
           Serial.println(buffer);
           Serial.flush();
        }
        pixels = infoheader + biSize + paletteSize * 4;
    } else if (biBitCount == 24) {
      // no palette
      Serial.println("No palette");
      pixels = infoheader + biSize; // TODO offset ???
//      pixels = data + offset; 
    } else {
      Serial.println("Not supported");
      return image;
    }
       uint8_t r, g, b;
     switch (biCompression) {
      case BMP_ENCODING_RGB: { // supported case 24 and 8 bits/pixel
           Serial.println("Reading RGB pixel data");
          for (int32_t y = 0; y < biHeight; ++y) {
            int32_t yy = reversed ? y : (biHeight - y - 1);
              const uint8_t* startRow = pixels + y * bytesPerRow;
              int32_t cpixel = 0;
              for (int32_t x = 0; x < biWidth; ++x) {
                  if (biBitCount == 24) {
                      r = startRow[x * 3 + 2]; g = startRow[x * 3 + 1]; b = startRow[x * 3 + 0];
                       setBMPColor(image, x, yy, CRGB(r, g, b));
                  } else if (biBitCount == 8) { // direct index to palette                      
                      uint8_t index = startRow[x];
                      const uint8_t* paletteEntry = palette + 4 * index; 
                      r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
                      setBMPColor(image, x, yy, CRGB(r, g, b));
                  } else if (biBitCount == 4) {
                      uint8_t index1 = startRow[cpixel] >> 4;
                      uint8_t index2 = startRow[cpixel] & 0xf;
                      const uint8_t* paletteEntry1 = palette + 4 * index1;
                      r = paletteEntry1[2]; g = paletteEntry1[1]; b = paletteEntry1[0];
                      // emit first pixel
                      setBMPColor(image, x, yy, CRGB(r, g, b));
                      ++x;
                      if (x < biWidth) {
                            // emit the second pixel
                            const uint8_t* paletteEntry2 = palette + 4 * index2;
                            r = paletteEntry2[2]; g = paletteEntry2[1]; b = paletteEntry2[0];
                            setBMPColor(image, x, yy, CRGB(r, g, b));
                      }
                      cpixel++;
                  }
              }
          }
          break;
      }
      case BMP_ENCODING_RLE8: {
           Serial.println("Reading RLE pixel data");
           int32_t x = 0; int32_t y = 0;
           boolean decode = true;
           while (decode == true) {
            uint8_t length = *pixels++;
            if (length == 0) {
              uint8_t code = *pixels++;
               switch (code) {
                case RLE_EOL: { // end of line
                  x = 0; y++;
                  break;
                }
                case RLE_EOB: { // end of file
                  decode = false;
                  break;
                }
                case RLE_DELTA: { // offset mode
                  x += *pixels++; y += *pixels++;
                  break;
                }
                default: { // literal mode
                  length = *pixels++;
                  for (uint8_t idx = 0; idx < length; ++idx) {
                    if (x >= biWidth) {
                      x = 0; y++;
                    }
                    // use literal value
                    uint8_t index = *pixels++;
                    const uint8_t* paletteEntry = palette + 4 * index; 
                    r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
                     setBMPColor(image, x, y, CRGB(r, g, b));
                    x++;
                  }
                  if (length & 1) // fill bye
                    pixels++;
                }
              }
           } else { // normal run
            uint8_t index = *pixels++;
            for (uint8_t idx = 0; idx < length; ++idx) {
              if (x >= biWidth) {
                x = 0; y++;
              }
              const uint8_t* paletteEntry = palette + 4 * index; 
              r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
              setBMPColor(image, x, y, CRGB(r, g, b));
            } 
            x++;
            if ((x == biWidth) && (y == biHeight)) {
             decode = false;         
            }
           }
        }
        break;
      }
    }
    Serial.println("Done");
    return image;
}

struct BMPImage* decodeBMPFile(File file) {
    uint8_t header[BMP_FILE_HEADER_SIZE];
    uint8_t fourbytes[4];
      uint8_t r, g, b;
  if (file.size() <= BMP_FILE_HEADER_SIZE)
    return NULL;
    Serial.println(); 
  file.readBytes((char*)header, BMP_FILE_HEADER_SIZE);
  if (header[0] != 'B' || header[1] != 'M') 
    return NULL;
  uint32_t offset = read_uint32(header, 10);
//  Serial.print("offset "); Serial.println(offset);
  // now we are at BITMAPINFOHEADER POS
  // read size from file
  file.readBytes((char*)fourbytes, 4);
  // rear from memory
  uint32_t biSize = read_uint32(fourbytes, 0);
//  Serial.print("biSize "); Serial.println(biSize);
  // allocate data for BITMAPINFOHEADER and read
  uint8_t* infoheader = (uint8_t*)malloc(biSize - 4);
  file.readBytes((char*)infoheader, biSize - 4);

  // read normally from memory, starting from width
   int32_t biWidth = read_int32(infoheader, 0);
//    Serial.print("biWidth "); Serial.println(biWidth);
     // negative means top down instead of bottom up
    int32_t biHeight = read_int32(infoheader, 4);
//    Serial.print("biHeight "); Serial.println(biHeight);
    boolean reversed = false;
    if (biHeight < 0) {
      reversed = true;
      biHeight = -biHeight;
    }
    struct BMPImage* image = allocateBMP(biWidth, biHeight);
    uint16_t biBitCount = read_uint16(infoheader, 10);
//    Serial.print("biBitCount "); Serial.println(biBitCount);
   int32_t biCompression = read_int32(infoheader, 12);
 //   Serial.print("biCompression "); Serial.println(biCompression);
    uint32_t bytesPerRow = ((biWidth * biBitCount + 31) / 32) * 4;
   //Serial.print("bytesPerRow "); Serial.println(bytesPerRow);
    // no need for BIH
    free(infoheader);
    uint8_t* palette = 0;
    if ((biBitCount == 8) || (biBitCount == 4)) { 
      uint16_t paletteSize = (uint16_t)(1 << biBitCount); // 8 supported
      //Serial.print("Palette size "); Serial.println(paletteSize);
      // read palette from file
      palette = (uint8_t*)malloc(paletteSize * 4);
      // read normally from memory
      file.readBytes((char*)palette, paletteSize * 4);
      // display palette RGBQUAD
      // char buffer[64];     
      //for (uint16_t idx = 0; idx < paletteSize; ++idx) {
      //  sprintf(buffer, "entry %d : %d %d %d", idx, palette[4 * idx + 2], palette[4 * idx + 1], palette[4 * idx + 0]);
       // Serial.println(buffer);
      //}
    } else if (biBitCount == 24) {
      // no palette
 //     Serial.println("No palette");
    } else {
      Serial.println("Not supported");
      return image;
    }
    // pixels follow at offset
    file.seek(offset, SeekSet);
     switch (biCompression) {
      case BMP_ENCODING_RGB: { // supported case 24 and 8 bits/pixel
        // read data
 //       Serial.println("Reading pixel data");
            // allocating one line
        uint8_t* startRow = (uint8_t*)malloc(bytesPerRow);
        for (int32_t y = 0; y < biHeight; ++y) {
          int32_t yy = reversed ? y : (biHeight - y - 1);
          file.readBytes((char*)startRow, bytesPerRow);
         // then parse data in memory
         if (biBitCount == 24) { 
            for (int32_t x = 0; x < biWidth; ++x) {            
                r = startRow[x * 3 + 2]; g = startRow[x * 3 + 1]; b = startRow[x * 3 + 0];
               setBMPColor(image, x, yy, CRGB(r, g, b));
            }
          } else if (biBitCount == 8) {  // direct index to palette 
            for (int32_t x = 0; x < biWidth; ++x) { 
                  uint16_t index = startRow[x];
                  uint8_t* paletteEntry = palette + 4 * index; 
                  r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
                   setBMPColor(image, x, yy, CRGB(r, g, b)); 
            }
          } else if (biBitCount == 4) {
            int32_t cpixel = 0;
            for (int32_t x = 0; x < biWidth; ++x) {
                uint8_t index1 = startRow[cpixel] >> 4;
                uint8_t index2 = startRow[cpixel] & 0xf;
                uint8_t* paletteEntry1 = palette + 4 * index1;
                r = paletteEntry1[2]; g = paletteEntry1[1]; b = paletteEntry1[0];
                // emit first pixel
                 setBMPColor(image, x, yy, CRGB(r, g, b));
                ++x;
                if (x < biWidth) {
                  // emit the second pixel
                  uint8_t* paletteEntry2 = palette + 4 * index2;
                  r = paletteEntry2[2]; g = paletteEntry2[1]; b = paletteEntry2[0];
                  setBMPColor(image, x, yy, CRGB(r, g, b));
                }
                cpixel++;
             }
           }
        }
       free(startRow);
         break;
      }
      case BMP_ENCODING_RLE8: {
           Serial.println("Reading RLE pixel data");
           int32_t x = 0; int32_t y = 0;
           boolean decode = true;
           while (decode == true) {
                uint8_t length;
                file.readBytes((char*)&length, 1);
                if (length == 0) {
                  uint8_t code;
                  file.readBytes((char*)&code, 1);
                  switch (code) {
                    case RLE_EOL: { // end of line
                      x = 0; y++;
                      break;
                    }
                    case RLE_EOB: { // end of file
                      decode = false;
                      break;
                    }
                    case RLE_DELTA: { // offset mode
                      uint8_t delta[2];
                      file.readBytes((char*)delta, 2);
                      x += delta[0]; y += delta[1];
                      break;
                    } 
                    default: { // literal mode
                      file.readBytes((char*)&length, 1);
                      for (uint8_t idx = 0; idx < length; ++idx) {
                        if (x >= biWidth) {
                          x = 0; y++;
                        }
                        // use literal value
                        uint8_t index;
                        file.readBytes((char*)&index, 1);
                        const uint8_t* paletteEntry = palette + 4 * index; 
                        r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
                        int32_t yy = reversed ? y : (biHeight - y - 1);
                         setBMPColor(image, x, yy, CRGB(r, g, b));
                        x++;
                      }
                      if (length & 1) // fill byte
                        file.readBytes((char*)index, 1); // not used
                    }
                  }
              } else { // normal run
                  uint8_t index;
                  file.readBytes((char*)&index, 1);
                  for (uint8_t idx = 0; idx < length; ++idx) {
                      if (x >= biWidth) {
                        x = 0; y++;
                      }
                      x++;
                      const uint8_t* paletteEntry = palette + 4 * index; 
                      r = paletteEntry[2]; g = paletteEntry[1]; b = paletteEntry[0];
                      int32_t yy = reversed ? y : (biHeight - y - 1);
                      setBMPColor(image, x, yy, CRGB(r, g, b));
                  } 
                  x++;
                  if (y >= biHeight) {
                      decode = false;         
                  }
              } 
           }      
        break;
      }
    } 
    if (palette)
      free(palette);
     return image;
 }

struct BMPImage* bmpInvaderImage;
struct BMPImage* bmpInvader2Image;

void setup() {
   Serial.begin(baud);
   Serial.println();
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, numleds).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(7);
  FastLED.clear();
  FastLED.show();
  delay(5000);
  
  bmpInvaderImage = decodeBMPInMemory(data_invaderApp_4_8x8_bmp);
  bmpInvader2Image = decodeBMPInMemory(data_invader_4_8x8_bmp);
  if (!SPIFFS.begin())
    Serial.println("No SPIFFS");
}

void decodeFile(const char* name) {
  Serial.print(name);
//  FastLED.clear();
  File f = SPIFFS.open(name, "r");
  if (f) {
    struct BMPImage* image = decodeBMPFile(f);
    f.close();
    if (image) {
      for (uint8_t xpos = 0; xpos < image->width; xpos++) {
        
// Move old image left
        for (uint8_t j=0; j<height; j++)
          for (uint8_t l=0; (width -1); l++)
            if (XY(l, j)<256) 
              leds[XY(l, j)]=leds[XY(l+1, j)];

        displayBMP(image, width - 1 - xpos, 0);
        FastLED.show();
        delay(50);
      }
/*
      if(image->height==16)
      {
      delay(20);
      for (uint8_t xpos = 0; xpos < image->width; xpos++) {
        
// Move old image left
        for (uint8_t j=0; j<height; j++)
          for (uint8_t l=0; l<(width -1); l++)
            if (XY(l, j)<256) 
              leds[XY(l, j)]=leds[XY(l+1, j)];

        displayBMP2(image, 7 - xpos, 0,8);
        FastLED.show();
        delay(50);
      }
      }
      */
      freeBMP(image);
    }
  } else {
    Serial.print(" not found");
  }
  Serial.println();
  FastLED.show();
}

void decodeFileLemming(const char* name) {
  Serial.print(name);
  FastLED.clear();
  File f = SPIFFS.open(name, "r");
  if (f) {
    struct BMPImage* image = decodeBMPFile(f);
    f.close();
    if (image) {
        displayBMP(image, -1, 0);
        freeBMP(image);
    }
  } else {
    Serial.print(" not found");
  }
  Serial.println();
  FastLED.show();
}

void loop() {
#if 1
  FastLED.clear();
  displayBMP(bmpInvaderImage, 0, 0);
  FastLED.show();
 delay(5000);
  FastLED.clear();
  displayBMP(bmpInvader2Image, 0, 0);
  FastLED.show();
 delay(5000);
#endif

#if 1
// display sprites0
   for (uint8_t idx = 0; idx < 16; ++idx) {
        char buffer[32];     
       sprintf(buffer, "/16x16/bubble%02d.bmp", idx);
        decodeFile(buffer); 
        delay(1000); 
   }
#endif

#if 1
// display sprites0
   for (uint8_t idx = 0; idx < 100; ++idx) {
        char buffer[32];     
       sprintf(buffer, "/sprites/sprites_%02d.bmp", idx);
        decodeFile(buffer); 
        delay(1000); 
   }
#endif
#if 1
    // display lemmings anim
  for (uint8_t loop = 0; loop < 20; ++loop) {
     for (uint8_t idx = 0; idx < 8; ++idx) {
          char buffer[32];     
         sprintf(buffer, "/lemmings/lemming_%d.bmp", idx);
          decodeFileLemming(buffer); 
          delay(100); 
     }
    }
#endif
}
