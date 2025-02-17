#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

static const char BMP_SIG_B = 'B';
static const char BMP_SIG_M = 'M';
static const int BMP_HEADER_SIZE = 54;
static const uint32_t BMP_RESERVED_SPACE = 0;
static const uint32_t BMP_INDENT = BMP_HEADER_SIZE;

static const uint32_t BMP_INFO_HEADER_SIZE = 40;
static const uint16_t BMP_NUMBER_OF_PLANES = 1;
static const uint16_t BMP_BITS_PER_PIXEL = 24;
static const uint32_t BMP_COMPRESSION_TYPE = 0;
static const int BMP_VERTICAL_RESOLUTION = 11811;
static const int BMP_HORIZONTAL_RESOLUTION = 11811;
static const int BMP_NUMBER_OF_COLORS = 0;
static const int BMP_NUMBER_OF_SIGNIFICANT_COLORS = 0x1000000;

static const int BYTES_PER_PIXEL = 3;
static const int BMP_ALIGNMENT = 4;

PACKED_STRUCT_BEGIN BitmapFileHeader {
    char b;
    char m;
    uint32_t header_and_data_size;
    uint32_t reserved_space;
    uint32_t indent;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size;
    int image_width;
    int image_height;
    uint16_t number_of_planes;
    uint16_t bits_per_pixel;
    uint32_t type_of_compress;
    uint32_t bytes_in_data;
    int horizontal_resolution;
    int vertical_resolution;
    int number_of_colors;
    int number_of_significant_colors;
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return BMP_ALIGNMENT * ((w * BYTES_PER_PIXEL + BYTES_PER_PIXEL) / BMP_ALIGNMENT);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);

    BitmapFileHeader file_header;
    file_header.b = BMP_SIG_B;
    file_header.m = BMP_SIG_M;
    file_header.header_and_data_size = GetBMPStride(image.GetWidth()) * image.GetHeight() + BMP_HEADER_SIZE;
    file_header.reserved_space = BMP_RESERVED_SPACE;
    file_header.indent = BMP_INDENT;

    BitmapInfoHeader info_header;
    info_header.header_size = BMP_INFO_HEADER_SIZE;
    info_header.image_width = image.GetWidth();
    info_header.image_height = image.GetHeight();
    info_header.number_of_planes = BMP_NUMBER_OF_PLANES;
    info_header.bits_per_pixel = BMP_BITS_PER_PIXEL;
    info_header.type_of_compress = BMP_COMPRESSION_TYPE;
    info_header.bytes_in_data = GetBMPStride(image.GetWidth()) * image.GetHeight();
    info_header.horizontal_resolution = BMP_HORIZONTAL_RESOLUTION;
    info_header.vertical_resolution = BMP_VERTICAL_RESOLUTION;
    info_header.number_of_colors = BMP_NUMBER_OF_COLORS;
    info_header.number_of_significant_colors = BMP_NUMBER_OF_SIGNIFICANT_COLORS;

    out.write(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader));
    out.write(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader));

    std::vector<char> buff(GetBMPStride(image.GetWidth()));

    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for(int x = 0; x < image.GetWidth(); ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buff.data(), GetBMPStride(image.GetWidth()));
    }
    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    ifs.read(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader));
    ifs.read(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader));
    
    if (ifs.bad()) {
        return {};
    }

    if (file_header.b != BMP_SIG_B || file_header.m != BMP_SIG_M || file_header.indent != BMP_INDENT || file_header.reserved_space != BMP_RESERVED_SPACE 
        || file_header.header_and_data_size != GetBMPStride(info_header.image_width) * info_header.image_height + BMP_HEADER_SIZE
        || info_header.header_size != BMP_INFO_HEADER_SIZE || info_header.number_of_planes != BMP_NUMBER_OF_PLANES || info_header.bits_per_pixel != BMP_BITS_PER_PIXEL
        || info_header.type_of_compress != BMP_COMPRESSION_TYPE 
        || info_header.bytes_in_data != GetBMPStride(info_header.image_width) * info_header.image_height
        || info_header.horizontal_resolution != BMP_HORIZONTAL_RESOLUTION || info_header.vertical_resolution != BMP_VERTICAL_RESOLUTION
        || info_header.number_of_colors != BMP_NUMBER_OF_COLORS || info_header.number_of_significant_colors != BMP_NUMBER_OF_SIGNIFICANT_COLORS) {
            return {};
    }
    
    Image image(info_header.image_width, info_header.image_height, Color::Black());

    std::vector<char> buff(GetBMPStride(info_header.image_width));

    for (int y = info_header.image_height - 1; y >= 0; --y) {
        Color* line = image.GetLine(y);
        ifs.read(buff.data(), GetBMPStride(info_header.image_width));
        if (ifs.bad()) {
           return {};
        }
        for (int x = 0; x < info_header.image_width; ++x) {
            line[x].b = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
        }
    }
    return image;
}
}  // namespace img_lib
