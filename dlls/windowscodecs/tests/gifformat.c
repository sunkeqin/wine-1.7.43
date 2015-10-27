/*
 * Copyright 2012 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "wincodec.h"
#include "wine/test.h"

HRESULT WINAPI WICCreateImagingFactory_Proxy(UINT, IWICImagingFactory**);

static const char gif_global_palette[] = {
/* LSD */'G','I','F','8','7','a',0x01,0x00,0x01,0x00,0xa1,0x02,0x00,
/* palette */0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,
/* GCE */0x21,0xf9,0x04,0x01,0x05,0x00,0x01,0x00, /* index 1 */
/* IMD */0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
0x02,0x02,0x44,0x01,0x00,0x3b
};

/* frame 0, GCE transparent index 1
 * frame 1, GCE transparent index 2
 */
static const char gif_global_palette_2frames[] = {
/* LSD */'G','I','F','8','9','a',0x01,0x00,0x01,0x00,0xa1,0x02,0x00,
/* palette */0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,
/* GCE */0x21,0xf9,0x04,0x01,0x05,0x00,0x01,0x00, /* index 1 */
/* IMD */0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
0x02,0x02,0x44,0x01,0x00,
/* GCE */0x21,0xf9,0x04,0x01,0x05,0x00,0x02,0x00, /* index 2 */
/* IMD */0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
0x02,0x02,0x44,0x01,0x00,0x3b
};

static const char gif_local_palette[] = {
/* LSD */'G','I','F','8','7','a',0x01,0x00,0x01,0x00,0x27,0x02,0x00,
/* GCE */0x21,0xf9,0x04,0x01,0x05,0x00,0x01,0x00, /* index 1 */
/* IMD */0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x81,
/* palette */0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,
0x02,0x02,0x44,0x01,0x00,0x3b
};

/* Generated with ImageMagick:
 * convert -delay 100 -size 2x2 xc:red \
 *     -dispose none -page +0+0 -size 2x1 xc:white \
 *     test.gif
 */
static const char gif_frame_sizes[] = {
    0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x02, 0x00,
    0x02, 0x00, 0xf1, 0x00, 0x00, 0xff, 0x00, 0x00,
    0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00,
    0x00, 0x21, 0xf9, 0x04, 0x00, 0x64, 0x00, 0x00,
    0x00, 0x21, 0xff, 0x0b, 0x4e, 0x45, 0x54, 0x53,
    0x43, 0x41, 0x50, 0x45, 0x32, 0x2e, 0x30, 0x03,
    0x01, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03,
    0x44, 0x34, 0x05, 0x00, 0x21, 0xf9, 0x04, 0x04,
    0x64, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x01, 0x00, 0x80, 0xff, 0xff,
    0xff, 0x00, 0x00, 0x00, 0x02, 0x02, 0x04, 0x0a,
    0x00, 0x3b
};

static IWICImagingFactory *factory;

static IWICBitmapDecoder *create_decoder(const void *image_data, UINT image_size)
{
    HGLOBAL hmem;
    BYTE *data;
    HRESULT hr;
    IWICBitmapDecoder *decoder = NULL;
    IStream *stream;
    GUID format;
    LONG refcount;

    hmem = GlobalAlloc(0, image_size);
    data = GlobalLock(hmem);
    memcpy(data, image_data, image_size);
    GlobalUnlock(hmem);

    hr = CreateStreamOnHGlobal(hmem, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal error %#x\n", hr);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, &decoder);
    ok(hr == S_OK, "CreateDecoderFromStream error %#x\n", hr);

    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &format);
    ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
       "wrong container format %s\n", wine_dbgstr_guid(&format));

    refcount = IStream_Release(stream);
    ok(refcount > 0, "expected stream refcount > 0\n");

    return decoder;
}

static void test_global_gif_palette(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;
    UINT count, ret;
    WICColor color[256];

    decoder = create_decoder(gif_global_palette, sizeof(gif_global_palette));
    ok(decoder != 0, "Failed to load GIF image data\n");

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);

    /* global palette */
    hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    /* frame palette */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "wrong pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_global_gif_palette_2frames(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;
    UINT count, ret;
    WICColor color[256];

    decoder = create_decoder(gif_global_palette_2frames, sizeof(gif_global_palette_2frames));
    ok(decoder != 0, "Failed to load GIF image data\n");

    /* active frame 0, GCE transparent index 1 */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);

    /* global palette */
    hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    /* frame 0 palette */
    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "wrong pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    IWICBitmapFrameDecode_Release(frame);

    /* active frame 1, GCE transparent index 2 */
    hr = IWICBitmapDecoder_GetFrame(decoder, 1, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    /* global palette */
    hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0xff040506 || broken(color[1] == 0x00040506) /* XP */, "expected 0xff040506, got %#x\n", color[1]);
    ok(color[2] == 0x00070809 || broken(color[2] == 0xff070809) /* XP */, "expected 0x00070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    /* frame 1 palette */
    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "wrong pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0xff040506, "expected 0xff040506, got %#x\n", color[1]);
    ok(color[2] == 0x00070809, "expected 0x00070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_local_gif_palette(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    WICBitmapPaletteType type;
    GUID format;
    UINT count, ret, i;
    WICColor color[256];

    decoder = create_decoder(gif_local_palette, sizeof(gif_local_palette));
    ok(decoder != 0, "Failed to load GIF image data\n");

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);

    /* global palette */
    hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
    ok(hr == S_OK || broken(hr == WINCODEC_ERR_FRAMEMISSING), "CopyPalette %#x\n", hr);
    if (hr == S_OK)
    {
        type = -1;
        hr = IWICPalette_GetType(palette, &type);
        ok(hr == S_OK, "GetType error %#x\n", hr);
        ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %#x\n", type);

        hr = IWICPalette_GetColorCount(palette, &count);
        ok(hr == S_OK, "GetColorCount error %#x\n", hr);
        ok(count == 256, "expected 256, got %u\n", count);

        hr = IWICPalette_GetColors(palette, count, color, &ret);
        ok(hr == S_OK, "GetColors error %#x\n", hr);
        ok(ret == count, "expected %u, got %u\n", count, ret);
        ok(color[0] == 0xff000000, "expected 0xff000000, got %#x\n", color[0]);
        ok(color[1] == 0x00ffffff, "expected 0x00ffffff, got %#x\n", color[1]);

        for (i = 2; i < 256; i++)
            ok(color[i] == 0xff000000, "expected 0xff000000, got %#x\n", color[i]);
    }

    /* frame palette */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "wrong pixel format %s\n", wine_dbgstr_guid(&format));

    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    type = -1;
    hr = IWICPalette_GetType(palette, &type);
    ok(hr == S_OK, "GetType error %#x\n", hr);
    ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %#x\n", type);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_gif_frame_sizes(void)
{
    static const BYTE frame0[] = {0, 1, 0xfe, 0xfe, 2, 3, 0xfe, 0xfe};
    static const BYTE frame1[] = {0, 0, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe};

    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    UINT width, height;
    BYTE buf[8];
    HRESULT hr;

    decoder = create_decoder(gif_frame_sizes, sizeof(gif_frame_sizes));
    ok(decoder != 0, "Failed to load GIF image data\n");

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetSize(frame, &width, &height);
    ok(hr == S_OK, "GetSize error %x\n", hr);
    ok(width == 2, "width = %d\n", width);
    ok(height == 2, "height = %d\n", height);

    memset(buf, 0xfe, sizeof(buf));
    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, 4, sizeof(buf), buf);
    ok(hr == S_OK, "CopyPixels error %x\n", hr);
    ok(!memcmp(buf, frame0, sizeof(buf)), "buf = %x %x %x %x %x %x %x %x\n",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    IWICBitmapFrameDecode_Release(frame);

    hr = IWICBitmapDecoder_GetFrame(decoder, 1, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetSize(frame, &width, &height);
    ok(hr == S_OK, "GetSize error %x\n", hr);
    ok(width == 2, "width = %d\n", width);
    ok(height == 1, "height = %d\n", height);

    memset(buf, 0xfe, sizeof(buf));
    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, 4, sizeof(buf), buf);
    ok(hr == S_OK, "CopyPixels error %x\n", hr);
    ok(!memcmp(buf, frame1, sizeof(buf)), "buf = %x %x %x %x %x %x %x %x\n",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    IWICBitmapFrameDecode_Release(frame);

    IWICBitmapDecoder_Release(decoder);
}

START_TEST(gifformat)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    if (FAILED(hr)) return;

    test_global_gif_palette();
    test_global_gif_palette_2frames();
    test_local_gif_palette();
    test_gif_frame_sizes();

    IWICImagingFactory_Release(factory);
    CoUninitialize();

    /* run the same tests with no COM initialization */
    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    ok(hr == S_OK, "WICCreateImagingFactory_Proxy error %#x\n", hr);

    test_global_gif_palette();
    test_global_gif_palette_2frames();
    test_local_gif_palette();
    test_gif_frame_sizes();

    IWICImagingFactory_Release(factory);
}
