#include "dbmo/display/framebuffer_display.h"

#include <cstdio>

namespace dbmo::display {

FramebufferDisplay::FramebufferDisplay(Color *pixels, uint16_t width, uint16_t height)
    : pixels_(pixels), width_(width), height_(height) {}

Status FramebufferDisplay::takeFault() {
  const Status status = nextStatus_;
  nextStatus_ = Status::Ok;
  return status;
}

Status FramebufferDisplay::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
  if (!isOk(nextStatus_)) return takeFault();
  ++fillRectCalls;
  if (w <= 0 || h <= 0) return Status::Ok;

  // Recorte em 32 bits: x + w pode passar de int16.
  int32_t left = x;
  int32_t top = y;
  int32_t right = static_cast<int32_t>(x) + w;
  int32_t bottom = static_cast<int32_t>(y) + h;
  if (left < 0) left = 0;
  if (top < 0) top = 0;
  if (right > width_) right = width_;
  if (bottom > height_) bottom = height_;

  for (int32_t row = top; row < bottom; ++row) {
    Color *line = pixels_ + static_cast<size_t>(row) * width_;
    for (int32_t column = left; column < right; ++column) {
      line[column] = color;
      ++pixelsWritten;
    }
  }
  return Status::Ok;
}

Status FramebufferDisplay::setBacklight(uint8_t percent) {
  if (!isOk(nextStatus_)) return takeFault();
  if (!backlightSupported) return Status::Unsupported;
  if (percent > 100) return Status::OutOfRange;
  backlightPercent = percent;
  return Status::Ok;
}

Status FramebufferDisplay::setSleep(bool sleep) {
  if (!isOk(nextStatus_)) return takeFault();
  asleep = sleep;
  return Status::Ok;
}

void rgb565ToRgb888(Color color, uint8_t rgb[3]) {
  const uint8_t r5 = static_cast<uint8_t>((color >> 11) & 0x1F);
  const uint8_t g6 = static_cast<uint8_t>((color >> 5) & 0x3F);
  const uint8_t b5 = static_cast<uint8_t>(color & 0x1F);
  rgb[0] = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
  rgb[1] = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
  rgb[2] = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
}

Status writePpm(const Color *pixels, uint16_t width, uint16_t height, ByteSink sink,
                void *context) {
  if (pixels == nullptr || sink == nullptr || width == 0 || height == 0)
    return Status::InvalidArgument;

  char header[32];
  const int headerLength = snprintf(header, sizeof(header), "P6\n%u %u\n255\n",
                                    static_cast<unsigned>(width), static_cast<unsigned>(height));
  if (!sink(reinterpret_cast<const uint8_t *>(header), static_cast<size_t>(headerLength), context))
    return Status::IoError;

  // Uma linha por vez, em blocos pequenos: não exige um buffer do tamanho da imagem.
  uint8_t chunk[96];
  for (uint32_t row = 0; row < height; ++row) {
    size_t used = 0;
    for (uint32_t column = 0; column < width; ++column) {
      rgb565ToRgb888(pixels[static_cast<size_t>(row) * width + column], chunk + used);
      used += 3;
      if (used == sizeof(chunk) || column + 1 == width) {
        if (!sink(chunk, used, context)) return Status::IoError;
        used = 0;
      }
    }
  }
  return Status::Ok;
}

}  // namespace dbmo::display
