#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

// clamp v between interval [low, high]
static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void drawWrappedText(const char* text, int x, int y, int maxW, int lineH) {
  String s(text);
  int line = 0;
  while (s.length() > 0) {
    int cut = s.length();
    while (cut > 0 && spr.textWidth(s.substring(0, cut)) > maxW) {
      cut--;
    }
    if (cut == 0) break;

    int lastSpace = s.substring(0, cut).lastIndexOf(' ');
    if (lastSpace > 0) cut = lastSpace;

    spr.drawString(s.substring(0, cut), x, y + line * lineH);
    s = s.substring(cut);
    s.trim();
    line++;
    if (line >= 2) break;
  }
}

void drawBoat(int x, int y, int headingDeg, int size, uint16_t fill) {
  float a = headingDeg * 3.1415926f / 180.0f;

  int x1 = x + (int)(size *  sin(a));
  int y1 = y - (int)(size *  cos(a));

  int x2 = x + (int)((size * 0.8f) * sin(a + 2.7f));
  int y2 = y - (int)((size * 0.8f) * cos(a + 2.7f));

  int x3 = x + (int)((size * 0.8f) * sin(a - 2.7f));
  int y3 = y - (int)((size * 0.8f) * cos(a - 2.7f));

  spr.fillTriangle(x1, y1, x2, y2, x3, y3, fill);
  spr.drawTriangle(x1, y1, x2, y2, x3, y3, TFT_BLACK);

  int xs = x - (int)((size * 0.9f) * sin(a));
  int ys = y + (int)((size * 0.9f) * cos(a));
  spr.drawLine(x, y, xs, ys, TFT_BLACK);
}

void drawWindFromMark(int cx, int yTop) {
  // wind comes down from mark
  spr.drawLine(cx, yTop, cx, yTop + 30, TFT_BLACK);
  spr.fillTriangle(cx, yTop + 30, cx - 6, yTop + 20, cx + 6, yTop + 20, TFT_BLACK);
  spr.setTextColor(TFT_BLACK, TFT_CYAN);
  spr.drawString("WIND", clampi(cx - 18, 2, tft.width() - 40), yTop + 34);
}

int frame = 0;

void setup() {
  tft.init();

  tft.setRotation(2);

  spr.createSprite(tft.width(), tft.height());
  spr.setTextDatum(TL_DATUM);
  spr.setTextSize(1);
}

void loop() {
  frame++;

  const int W = tft.width();
  const int H = tft.height();

  int boat = max(10, min(W, H) / 5);

  // cycle length: 120 frames
  float t = (frame % 120) / 119.0f;
  if (t > 0.5f) t = 1.0f - t;
  t *= 2.0f;

  spr.fillSprite(TFT_CYAN);
  
  // windward
  int markX = W / 2;
  int markY = 20;
  spr.fillCircle(markX, markY, 6, TFT_YELLOW);
  spr.drawCircle(markX, markY, 6, TFT_BLACK);
  spr.setTextColor(TFT_BLACK, TFT_CYAN);
  spr.drawString("MARK", clampi(markX - 18, 2, W - 40), markY - 14);

  drawWindFromMark(markX, markY + 8);

  // center passback animation
  int cx = W / 2;
  int cy = H / 2 + 15;

  // State A (setup): boats more separated
  int W2xA = cx - boat - 6, W2yA = cy - boat - 10;
  int M2xA = cx + 2,        M2yA = cy;
  int L2xA = cx + boat + 6, L2yA = cy + boat + 10;

  // State B (squeeze): W2 and L2 converge on M2; M2 barely advances upwind
  int W2xB = cx - boat/2 - 6, W2yB = cy - boat/2 - 2;
  int M2xB = cx + 2,          M2yB = cy - (int)(boat * 0.15f); // stalled: tiny progress upwind
  int L2xB = cx + boat/2 + 6, L2yB = cy + boat/2 + 6;

  int W2x = (int)(W2xA + (W2xB - W2xA) * t);
  int W2y = (int)(W2yA + (W2yB - W2yA) * t);
  int M2x = (int)(M2xA + (M2xB - M2xA) * t);
  int M2y = (int)(M2yA + (M2yB - M2yA) * t);
  int L2x = (int)(L2xA + (L2xB - L2xA) * t);
  int L2y = (int)(L2yA + (L2yB - L2yA) * t);

  // Headings: upwind is toward mark (top) => 0 degrees.
  // Show W2 luffing more as t increases; L2 pinching slightly.
  int up = 0;
  int W2hd = up + (int)(35 * t);   // luff up / slow
  int M2hd = up + (int)(24 * t);                   // trying to go straight
  int L2hd = up - (int)(20 * t);   // pinch

  // Draw "squeeze" lines
  spr.drawLine(W2x, W2y, M2x, M2y, TFT_DARKGREY);
  spr.drawLine(L2x, L2y, M2x, M2y, TFT_DARKGREY);

  // Dirty air / slow zone around M2 (simple ring)
  if (t > 0.35f) {
    spr.drawCircle(M2x, M2y, boat, TFT_DARKGREY);
    spr.drawCircle(M2x, M2y, boat + 2, TFT_DARKGREY);
  }

  drawBoat(W2x, W2y, W2hd, boat, TFT_BLUE);
  drawBoat(M2x, M2y, M2hd, boat, TFT_RED);
  drawBoat(L2x, L2y, L2hd, boat, TFT_BLUE);

  spr.setTextColor(TFT_BLACK, TFT_CYAN);
  spr.drawString("W2", W2x - 12, W2y - boat - 12);
  spr.drawString("M2", M2x - 12, M2y - boat - 12);
  spr.drawString("L2", L2x - 12, L2y - boat - 12);

  int footerH = 28; // taller footer to allow 2 lines
  spr.fillRect(0, H - footerH, W, footerH, TFT_LIGHTGREY);
  spr.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  
  const char* footerText =
    (t < 0.35f)
        ? "SETUP: close lanes"
        : "SQUEEZE: W2 luffs, L2 pinches to slow M2";
    
  drawWrappedText(footerText, 2, H - footerH + 3, W - 4, 12);

  spr.pushSprite(0, 0);
  delay(33);
}