#include "dbmo/display/text.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace dbmo::display {

namespace {

struct Colors {
  Color background;
  Color text;
};

Colors colorsOf(TextScreen::Style style) {
  switch (style) {
    case TextScreen::Style::Title: return {palette::kSignalRed, palette::kOffWhite};
    case TextScreen::Style::Selected: return {palette::kSilver, palette::kBlack};
    case TextScreen::Style::Hint: return {palette::kBlack, palette::kSilver};
    case TextScreen::Style::Normal: break;
  }
  return {palette::kBlack, palette::kOffWhite};
}

}  // namespace

Status drawText(IDisplay &display, int16_t x, int16_t y, const char *text, uint8_t scale,
                Color color, size_t maxChars) {
  if (text == nullptr || scale == 0) return Status::InvalidArgument;
  const int16_t advance = static_cast<int16_t>((kGlyphWidth + 1) * scale);
  for (size_t i = 0; text[i] != '\0' && i < maxChars; ++i, x = static_cast<int16_t>(x + advance)) {
    const uint8_t *rows = glyph(text[i]);
    for (uint8_t row = 0; row < kGlyphHeight; ++row) {
      // Cada sequência de pixels acesos na linha vira um retângulo só.
      uint8_t column = 0;
      while (column < kGlyphWidth) {
        if ((rows[row] & (0x10 >> column)) == 0) {
          ++column;
          continue;
        }
        const uint8_t start = column;
        while (column < kGlyphWidth && (rows[row] & (0x10 >> column)) != 0) ++column;
        DBMO_TRY(display.fillRect(
            static_cast<int16_t>(x + start * scale), static_cast<int16_t>(y + row * scale),
            static_cast<int16_t>((column - start) * scale), static_cast<int16_t>(scale), color));
      }
    }
  }
  return Status::Ok;
}

TextScreen::TextScreen(IDisplay &display) : display_(display) {
  const size_t fitColumns = display_.width() / kCellWidth;
  const size_t fitRows = display_.height() / kCellHeight;
  columns_ = fitColumns < kMaxColumns ? fitColumns : kMaxColumns;
  rows_ = fitRows < kMaxRows ? fitRows : kMaxRows;
  marginX_ = static_cast<int16_t>((display_.width() - columns_ * kCellWidth) / 2);
}

void TextScreen::clear() {
  for (Line &line : wanted_) line = Line{};
}

void TextScreen::setLine(size_t row, const char *text, Style style) {
  if (row >= rows_) return;
  Line &line = wanted_[row];
  snprintf(line.text, columns_ + 1, "%s", text != nullptr ? text : "");
  line.style = style;
}

void TextScreen::printLine(size_t row, Style style, const char *format, ...) {
  char text[kMaxColumns + 1];
  va_list args;
  va_start(args, format);
  vsnprintf(text, sizeof(text), format, args);
  va_end(args);
  setLine(row, text, style);
}

bool TextScreen::same(const Line &a, const Line &b) {
  return a.style == b.style && strcmp(a.text, b.text) == 0;
}

Status TextScreen::drawLine(size_t row) {
  const Colors colors = colorsOf(wanted_[row].style);
  const int16_t y = static_cast<int16_t>(row * kCellHeight);
  DBMO_TRY(display_.fillRect(0, y, static_cast<int16_t>(display_.width()), kCellHeight,
                             colors.background));
  // Meia célula de folga em cima e embaixo da letra.
  DBMO_TRY(drawText(display_, marginX_, static_cast<int16_t>(y + kScale / 2), wanted_[row].text,
                    kScale, colors.text, columns_));
  shown_[row] = wanted_[row];
  return Status::Ok;
}

Status TextScreen::render() {
  if (!valid_) {
    DBMO_TRY(display_.fillRect(0, 0, static_cast<int16_t>(display_.width()),
                               static_cast<int16_t>(display_.height()), palette::kBlack));
    for (Line &line : shown_) line = Line{};
    valid_ = true;
  }
  for (size_t row = 0; row < rows_; ++row) {
    if (same(wanted_[row], shown_[row])) continue;
    const Status status = drawLine(row);
    if (!isOk(status)) return status;  // a linha continua diferente: vai na próxima vez
  }
  return Status::Ok;
}

}  // namespace dbmo::display
