#include <iostream>
#include <string.h>
#include <cstdint>
#include <chrono>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#define USE_NEON
#elif defined(WIN32)
#include "NEON_2_SSE.h"
#define USE_NEON
#endif

using namespace std;


const int width = 360, height = 720;
void FullRange2VideoRangeNeon(uint8_t *data) {
    int size = width * height;
#ifdef USE_NEON
    // process Y data
    int slicePixelCount = 16; // neon: 16, sse: 4
    int divYPartCount = size / slicePixelCount;
    int endY = divYPartCount * slicePixelCount;
    int i = 0;
    for (; i < endY; i += slicePixelCount) {
        // load into register
        uint8x16_t y_16 = vld1q_u8(data + i);
        uint16x8_t y_16_l = vmovl_u8(vget_low_u8(y_16)); // 0~7bit
        uint16x8_t y_16_h = vmovl_u8(vget_high_u8(y_16)); // 8~15bit
        y_16_l = vmulq_n_u16(y_16_l, 219);
        y_16_h = vmulq_n_u16(y_16_h, 219);
        y_16_l = vshrq_n_u16(y_16_l, 8);
        y_16_h = vshrq_n_u16(y_16_h, 8);
        uint8x16_t result = vcombine_u16(vmovn_u16(y_16_l), vmovn_u16(y_16_h));
        result = vaddq_u8(result, vdupq_n_u8(16));
        // store into memory
        vst1q_u8(data + i, result);
    }

    // remaind Y data
    for (i = endY; i < size; ++i) {
        data[i] = (219 * data[i]) / 256 + 16;
    }

    // process UV data
    int divUVPartCount = (size / 2) / slicePixelCount;
    int endUV = size + divUVPartCount * slicePixelCount;
    for (i = size; i < endUV; i += slicePixelCount) {
        uint8x16_t uv_16 = vld1q_u8(data + i);
        uint16x8_t uv_16_l = vmovl_u8(vget_low_u8(uv_16)); // 0~7bit
        uint16x8_t uv_16_h = vmovl_u8(vget_high_u8(uv_16)); // 8~15bit
        uv_16_l = vmulq_n_u16(uv_16_l, 224);
        uv_16_h = vmulq_n_u16(uv_16_h, 224);
        uv_16_l = vshrq_n_u16(uv_16_l, 8);
        uv_16_h = vshrq_n_u16(uv_16_h, 8);
        uint8x16_t result = vcombine_u16(vmovn_u16(uv_16_l), vmovn_u16(uv_16_h));
        result = vaddq_u8(result, vdupq_n_u8(16));
        // store into memory
        vst1q_u8(data + i, result);
    }

    // remaind UV data
    for (i = endUV; i < size * 3 / 2; ++i) {
        data[i] = (224 * data[i]) / 256 + 16;
    }
#endif
}

void FullRange2VideoRangeNoNeon(uint8_t *data) {
    int size = width * height;
    // no acceleration version
    int i = 0;
    // Y
    for (; i < size; i++) {
        data[i] = (219 * data[i]) / 256 + 16;
    }
    // UV
    for (; i < size*3/2; i++) {
        data[i] = (224 * data[i]) / 256 + 16;
    }
}

int main() {
    int size = width * height;
    uint8_t data1[width * height * 3 / 2];
    uint8_t data2[width * height * 3 / 2];
    // Y
    for (int i = 0; i < size; ++i) {
        data1[i] = (uint8_t)(i % 256);
        data2[i] = (uint8_t)(i % 256);
    }
    // UV
    for (int i = 0; i < size / 2; ++i) {
        data1[size + i] = (uint8_t)(i % 256);
        data2[size + i] = (uint8_t)(i % 256);
    }
    // not use neon
    auto start1 = std::chrono::steady_clock::now();
    FullRange2VideoRangeNeon(data1);
    auto end1 = std::chrono::steady_clock::now();
    auto elapsed1 = std::chrono::duration_cast<std::chrono::microseconds>(end1-start1).count();

    // use neon
    auto start2 = std::chrono::steady_clock::now();
    FullRange2VideoRangeNoNeon(data2);
    auto end2 = std::chrono::steady_clock::now();
    auto elapsed2 = std::chrono::duration_cast<std::chrono::microseconds>(end2-start2).count();
    std::cout << "no NEON: "  << elapsed1 << "(us)" << std::endl;
    std::cout << "has NEON: " << elapsed2 << "(us)" << std::endl;

    // validate
    for (int i = 0; i < height * 3 / 2; ++i) {
        for (int j = 0; j < width; ++j) {
            if (data1[i * width + j] != data2[i * width + j]) {
                cout << "result is wrong!!!" << endl;
                return -1;
            }   
        }
    }
    return 0;
}