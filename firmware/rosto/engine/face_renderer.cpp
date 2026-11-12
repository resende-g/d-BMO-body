#include "dbmo/rosto/face_renderer.h"

namespace dbmo::rosto {

FaceRenderer::FaceRenderer(display::IDisplay &display) : display_(display) {
  for (auto &row : shown_)
    for (char &cell : row) cell = kPixelBackground;
}

uint16_t FaceRenderer::blockSize() const {
  const uint16_t byWidth = static_cast<uint16_t>(display_.width() / kGridWidth);
  const uint16_t byHeight = static_cast<uint16_t>(display_.height() / kGridHeight);
  return byWidth < byHeight ? byWidth : byHeight;
}

int16_t FaceRenderer::originX() const {
  return static_cast<int16_t>((display_.width() - blockSize() * kGridWidth) / 2);
}

int16_t FaceRenderer::originY() const {
  return static_cast<int16_t>((display_.height() - blockSize() * kGridHeight) / 2);
}

display::Color FaceRenderer::colorOf(char pixel) {
  if (pixel == kPixelSignal) return display::palette::kSignalRed;
  if (pixel == kPixelDark) return display::palette::kDarkRed;
  return display::palette::kBlack;
}

Status FaceRenderer::draw(const FrameView &view) {
  const int16_t block = static_cast<int16_t>(blockSize());
  if (block == 0) return Status::Unsupported;  // tela menor que a grade
  const int16_t x0 = originX();
  const int16_t y0 = originY();

  const bool full = !valid_;
  if (full) {
    const Status cleared =
        display_.fillRect(0, 0, static_cast<int16_t>(display_.width()),
                          static_cast<int16_t>(display_.height()), display::palette::kBlack);
    if (!isOk(cleared)) return cleared;
    for (auto &row : shown_)
      for (char &cell : row) cell = kPixelBackground;
  }
  // Enquanto o desenho não termina, o conteúdo da tela é desconhecido.
  valid_ = false;

  for (int y = 0; y < kGridHeight; ++y) {
    for (int x = 0; x < kGridWidth; ++x) {
      const char pixel = view.rows[y][x];
      if (pixel == shown_[y][x]) continue;
      const Status status =
          display_.fillRect(static_cast<int16_t>(x0 + x * block),
                            static_cast<int16_t>(y0 + y * block), block, block, colorOf(pixel));
      if (!isOk(status)) return status;
      shown_[y][x] = pixel;
    }
  }
  valid_ = true;
  return Status::Ok;
}

}  // namespace dbmo::rosto
