/*
    created with FontEditor written by H. Reddmann
    HaReddmann at t-online dot de

    File Name           : bigsym.h
    Date                : 06-03-10
    Font size in bytes  : 0x0080, 128
    Font width          : 23
    Font height         : 43
    Font first char     : 0x00
    Font last char      : 0x00
    Font bits per pixel : 1
    Font is compressed  : false

    The font data are defined as

    struct _FONT_ {
     // common shared fields
       uint16_t   font_Size_in_Bytes_over_all_included_Size_it_self;
       uint8_t    font_Width_in_Pixel_for_fixed_drawing;
       uint8_t    font_Height_in_Pixel_for_all_Characters;
       uint8_t    font_Bits_per_Pixels;
                    // if MSB are set then font is a compressed font
       uint8_t    font_First_Char;
       uint8_t    font_Last_Char;
       uint8_t    font_Char_Widths[font_Last_Char - font_First_Char +1];
                    // for each character the separate width in pixels,
                    // characters < 128 have an implicit virtual right empty row
                    // characters with font_Char_Widths[] == 0 are undefined

     // if compressed font then additional fields
       uint8_t    font_Byte_Padding;
                    // each Char in the table are aligned in size to this value
       uint8_t    font_RLE_Table[3];
                    // Run Length Encoding Table for compression
       uint8_t    font_Char_Size_in_Bytes[font_Last_Char - font_First_Char +1];
                    // for each char the size in (bytes / font_Byte_Padding) are stored,
                    // this get us the table to seek to the right beginning of each char
                    // in the font_data[].

     // for compressed and uncompressed fonts
       uint8_t    font_data[];
                    // bit field of all characters
    }
*/

//#include <inttypes.h>
//#include <avr/pgmspace.h>

#ifndef bigsym_H
#define bigsym_H

#define bigsym_WIDTH 23
#define bigsym_HEIGHT 43

const char bigsym[] = {
    0x00, 0x80, 0x17, 0x2B, 0x01, 0x00, 0x00,
    0x16, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x07, 0x00, 0x00, 
    0x00, 0xF0, 0xFF, 0x3F, 0x00, 0x00, 0x80, 0x07, 0xFE, 0xFF, 0x00, 0x00, 0x14, 0x00, 0xF0, 0xFF, 
    0x03, 0xA0, 0xF0, 0x0F, 0xC0, 0x1F, 0x00, 0x85, 0xFF, 0x30, 0xF8, 0x00, 0x2C, 0x58, 0x07, 0x2A, 
    0x07, 0x20, 0xC5, 0x0F, 0x54, 0x33, 0x00, 0x29, 0xFF, 0x2A, 0x92, 0x01, 0x68, 0xAB, 0x57, 0x9B, 
    0x0E, 0x60, 0xD9, 0x1F, 0x5A, 0xF4, 0x87, 0x09, 0xFE, 0xD5, 0xA0, 0xFD, 0xC7, 0xF0, 0xAF, 0x14, 
    0x2D, 0x3E, 0x86, 0x55, 0x2D, 0x68, 0x00, 0x3F, 0xF8, 0x5B, 0x61, 0x03, 0xF0, 0xFF, 0x1F, 0x00, 
    0x09, 0x00, 0xF0, 0xFF, 0xFF, 0xCC, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x07, 0x00, 0x00, 0x00, 0xE0, 
    0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD9
};

#endif

