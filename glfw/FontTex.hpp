#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <map>

typedef struct {
    int from_point;
    int to_point;
    int accum;
} char_range;

class FontTex {
public:
    static constexpr int num_ranges = 3;
    stbtt_pack_range pr[num_ranges];
    static constexpr int bitmap_width = 1024;
    static constexpr int bitmap_height = 1024;
    unsigned char bitmap[bitmap_width*bitmap_height];
    static constexpr int total_chars = 512;
    float font_size;
    float font_width;
    char_range char_ranges[num_ranges];
    float font_coords_tex[total_chars * 8];
    FontTex() : font_size(20.0f), font_width(12.0f) {
        unsigned char ttf_buffer[1<<20];
        fread(ttf_buffer, 1, 1<<20,
            fopen(std::string(std::string(PROJECT_ROOT) + "/fonts/Knack Regular Nerd Font Complete Mono.ttf").c_str(), "rb"));
        stbtt_pack_context pc;
        stbtt_PackBegin(&pc, bitmap, bitmap_width, bitmap_height, bitmap_width, 1, 0);
        stbtt_PackSetOversampling(&pc, 1, 1);
        stbtt_packedchar pchar[total_chars];
        // Char ranges
        // Basic latin
        char_ranges[0].from_point = 0x20;
        char_ranges[0].to_point = 0x7e;
        // latin-1-supplement
        char_ranges[1].from_point = 0xa0;
        char_ranges[1].to_point = 0xff;
        // dev
        char_ranges[2].from_point = 0xe700;
        char_ranges[2].to_point = 0xe7c5;
        int accum_chars = 0;
        for (int i = 0; i < num_ranges; ++i) {
            pr[i].chardata_for_range = pchar + accum_chars;
            pr[i].first_unicode_codepoint_in_range = char_ranges[i].from_point;
            pr[i].num_chars = char_ranges[i].to_point - char_ranges[i].from_point;
            pr[i].font_size = font_size;
            char_ranges[i].accum = accum_chars;
            accum_chars += pr[i].num_chars;
        }
        stbtt_PackFontRanges(&pc, ttf_buffer, 0, pr, num_ranges);
        //stbtt_packedchar cd[512];
        //stbtt_PackFontRange(&pc, ttf_buffer, 0, 17.0, 0, 512, cd);
        stbtt_PackEnd(&pc);
        stbi_write_png(std::string(std::string(PROJECT_ROOT) + "/font_test.png").c_str(), bitmap_width, bitmap_height, 1, bitmap, 0);
        // Texture data with positions for characters by internal code
        // Left x, top y, right x, bottom y, xoff, yoff, xadvance
        float *positions_pointer = font_coords_tex;
        for (int i = 0; i < num_ranges; ++i) {
            for (int j = 0; j < pr[i].num_chars; ++j) {
                const stbtt_packedchar *pchar = pr[i].chardata_for_range + j;
                *positions_pointer = (float)pchar->x0 / bitmap_width;
                ++positions_pointer;
                *positions_pointer = (float)pchar->y0 / bitmap_height;
                ++positions_pointer;
                *positions_pointer = (float)pchar->x1 / bitmap_width;
                ++positions_pointer;
                *positions_pointer = (float)pchar->y1 / bitmap_height;
                ++positions_pointer;

                *positions_pointer = pchar->xoff / bitmap_width;
                ++positions_pointer;
                *positions_pointer = pchar->yoff / bitmap_height;
                ++positions_pointer;
                *positions_pointer = pchar->xadvance / bitmap_width;
                ++positions_pointer;
                *positions_pointer = 0.0f;
                ++positions_pointer;
                fprintf(stdout, "code:%d, xoff:%f, yoff:%f, xoff2:%f, yoff2:%f\n", pr[i].first_unicode_codepoint_in_range+j,
                    pchar->xoff, pchar->yoff, pchar->xoff2, pchar->yoff2);
            }
        }
    }
    // Internal is continuous range usable from shader
    int codeToInternal(int code) {
        for (int i = 0; i < num_ranges; ++i) {
            if (code >= char_ranges[i].from_point && code <= char_ranges[i].to_point) {
                return char_ranges[i].accum + (code - char_ranges[i].from_point);
            }
        }
        // Return fallback
        return 0;
    }
};