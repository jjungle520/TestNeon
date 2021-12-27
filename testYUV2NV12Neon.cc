#include <iostream>
#include <string.h>
#include <cstdint>
#include <chrono>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#define USE_NEON
#elif defined(WIN32)
#include "NEON_2_SSE.h"
// #define USE_NEON
#endif

using namespace std;

void NV12TOYUV420P(const uint8_t* uv, uint8_t* u, uint8_t* v, int w, int h) {
	int size = w*h;
#ifdef USE_NEON
    int sliceDataCount = 16; // neon: 16, sse: 4
    int divPartCount = size / sliceDataCount;
	int divCountEnd = divPartCount * sliceDataCount;
	for (int i = 0; i < divCountEnd; i += sliceDataCount) {
		uint8x16x2_t uvData = vld2q_u8(uv + 2 * i);
		vst1q_u8(u + i, uvData.val[0]);
		vst1q_u8(v + i, uvData.val[1]);
	}
	// remained data
	 for (int i = divCountEnd; i < size; ++i) {
		u[i] = uv[2*i];
		v[i] = uv[2*i+1];
	 }
#else
	for (int i = 0; i < size; i++) {
		u[i] = uv[2*i];
		v[i] = uv[2*i+1];
	}
#endif
}

void YUV420PTONV12(const uint8_t* u, const uint8_t* v, uint8_t* uv, int w, int h) {
	int size = w*h;
#ifdef USE_NEON
	int sliceDataCount = 16; // neon: 16, sse: 4
    int divPartCount = size / sliceDataCount;
	int divCountEnd = divPartCount * sliceDataCount;
	for (int i = 0; i < divCountEnd; i += sliceDataCount) {
		uint8x16_t uData = vld1q_u8(u + i);
		uint8x16_t vData = vld1q_u8(v + i);
        uint8x16x2_t uvData;
        uvData.val[0] = uData;
        uvData.val[1] = vData;
		vst2q_u8(uv + 2 * i, uvData);
	}
	// remained data
	 for (int i = divCountEnd; i < size; ++i) {
		uv[2*i] = u[i];
		uv[2*i+1] = v[i];
	 }
#else
	for (int i = 0; i < size; i++) {
		uv[2*i] = u[i];
		uv[2*i+1] = v[i];
	}
#endif
}

void testNV12TOYUV420P() {
    cout << "split data" << endl;
    const uint64_t width = 10000, height = 10000;
    int size = width * height;
    uint8_t uv[width * height * 2];
    uint8_t u[width * height];
    uint8_t v[width * height];
    // uv
    for (int i = 0; i < size * 2; ++i) {
        uv[i] = (uint8_t)(i % 256);
    }

    for ( int i = 0; i < 20; ++i) {
        auto start2 = std::chrono::steady_clock::now();
        NV12TOYUV420P(uv,u,v,width,height);
        auto end2 = std::chrono::steady_clock::now();
        auto elapsed2 = std::chrono::duration_cast<std::chrono::microseconds>(end2-start2).count();
        std::cout << "NEON: " << elapsed2 << "(us)" << std::endl;
    }
}

void testYUV420PTONV12() {
    cout << "combine data" << endl;
    const uint64_t width = 10000, height = 10000;
    int size = width * height;
    uint8_t uv[width * height * 2];
    uint8_t u[width * height];
    uint8_t v[width * height];
    // u, v data
    for (int i = 0; i < size; ++i) {
        u[i] = (uint8_t)((2 * i) % 256);
        v[i] = (uint8_t)((2 * i + 1) % 256);
    }

    for ( int i = 0; i < 20; ++i) {
        auto start2 = std::chrono::steady_clock::now();
        YUV420PTONV12(u,v,uv,width,height);
        auto end2 = std::chrono::steady_clock::now();
        auto elapsed2 = std::chrono::duration_cast<std::chrono::microseconds>(end2-start2).count();
        std::cout << "NEON: " << elapsed2 << "(us)" << std::endl;
    }
}

int main() {
    // testNV12TOYUV420P();
    testYUV420PTONV12();
    return 0;
}