#include <Wire.h>
#include <math.h>

#define MAG_ADDR 0x1E

//type 'r'
bool running = false;

// sampling period (ms)
const uint32_t SAMPLE_PERIOD_MS = 100;
uint32_t lastSampleMs = 0;

const float ALPHA = 0.2f;

const float LSB_PER_GAUSS = 1090.0f;
const float GAUSS_TO_UT   = 100.0f;

// filtered values (in microtesla)
bool filtInit = false;
float fx_uT = 0, fy_uT = 0, fz_uT = 0;

// hard-iron calibration
bool calib = false;
int16_t minX = 32767, minY = 32767, minZ = 32767;
int16_t maxX = -32768, maxY = -32768, maxZ = -32768;
float offX = 0, offY = 0, offZ = 0;

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MAG_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(MAG_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MAG_ADDR, (int)len) != len) return false;
  for (int i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

// Read one magnetometer sample (raw counts)
bool readMagRaw(int16_t &mx, int16_t &my, int16_t &mz) {
  uint8_t b[6];
  if (!readRegs(0x03, b, 6)) return false;

  mx = (int16_t)((b[0] << 8) | b[1]);
  mz = (int16_t)((b[2] << 8) | b[3]);
  my = (int16_t)((b[4] << 8) | b[5]);
  return true;
}

void resetFilter() {
  filtInit = false;
  fx_uT = fy_uT = fz_uT = 0;
  Serial.println("# filter reset");
}

void calibStart() {
  calib = true;
  minX = minY = minZ = 32767;
  maxX = maxY = maxZ = -32768;
  Serial.println("# CALIB START: rotate the board slowly in all directions for ~10-20s");
  Serial.println("# Then press 'v' + Enter to compute offsets");
}

void calibFinish() {
  calib = false;
  offX = 0.5f * (float)(maxX + minX);
  offY = 0.5f * (float)(maxY + minY);
  offZ = 0.5f * (float)(maxZ + minZ);

  Serial.print("# OFFSETS (raw counts): offX,offY,offZ = ");
  Serial.print(offX, 2); Serial.print(",");
  Serial.print(offY, 2); Serial.print(",");
  Serial.println(offZ, 2);

  Serial.println("# Offsets are now applied BEFORE conversion to microtesla");
  resetFilter();
}

static inline float countsTo_uT(float counts) {
  return (counts / LSB_PER_GAUSS) * GAUSS_TO_UT;
}

void emitSample_uT(uint32_t t_ms, int16_t mx, int16_t my, int16_t mz) {
  // Update calibration min/max if active
  if (calib) {
    if (mx < minX) minX = mx; if (mx > maxX) maxX = mx;
    if (my < minY) minY = my; if (my > maxY) maxY = my;
    if (mz < minZ) minZ = mz; if (mz > maxZ) maxZ = mz;
  }

  // Apply offsets (hard-iron correction) in RAW counts
  float cx = (float)mx - offX;
  float cy = (float)my - offY;
  float cz = (float)mz - offZ;

  // Convert corrected values to microtesla
  float bx_uT = countsTo_uT(cx);
  float by_uT = countsTo_uT(cy);
  float bz_uT = countsTo_uT(cz);

  float B_uT = sqrtf(bx_uT * bx_uT + by_uT * by_uT + bz_uT * bz_uT);

  // EMA filter on microtesla axes
  if (!filtInit) {
    fx_uT = bx_uT; fy_uT = by_uT; fz_uT = bz_uT;
    filtInit = true;
  } else {
    fx_uT = ALPHA * bx_uT + (1.0f - ALPHA) * fx_uT;
    fy_uT = ALPHA * by_uT + (1.0f - ALPHA) * fy_uT;
    fz_uT = ALPHA * bz_uT + (1.0f - ALPHA) * fz_uT;
  }

  float Bf_uT = sqrtf(fx_uT * fx_uT + fy_uT * fy_uT + fz_uT * fz_uT);

  // Heading from filtered X/Y (not tilt-compensated)
  float heading = atan2f(fy_uT, fx_uT) * 180.0f / PI;
  if (heading < 0) heading += 360.0f;

  Serial.print(t_ms);
  Serial.print(",");
  Serial.print(bx_uT, 2); Serial.print(",");
  Serial.print(by_uT, 2); Serial.print(",");
  Serial.print(bz_uT, 2); Serial.print(",");
  Serial.print(B_uT, 2);  Serial.print(",");
  Serial.print(fx_uT, 2); Serial.print(",");
  Serial.print(fy_uT, 2); Serial.print(",");
  Serial.print(fz_uT, 2); Serial.print(",");
  Serial.print(Bf_uT, 2); Serial.print(",");
  Serial.println(heading, 1);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(21, 22);
  delay(200);

  writeReg(0x00, 0b01110000);
  writeReg(0x01, 0b00100000); 
  writeReg(0x02, 0x00);    

  Serial.println("=== HMC5883L STREAM (microtesla + EMA + HEADING) ===");
  Serial.println("Commands:");
  Serial.println("  r + Enter -> RUN (continuous)");
  Serial.println("  s + Enter -> STOP");
  Serial.println("  1 + Enter -> SINGLE READ (then stop)");
  Serial.println("  f + Enter -> RESET FILTER");
  Serial.println("  c + Enter -> START CALIB (min/max while rotating)");
  Serial.println("  v + Enter -> FINISH CALIB (compute offsets)");
  Serial.println();
  Serial.println("t_ms,bx_uT,by_uT,bz_uT,B_uT,fx_uT,fy_uT,fz_uT,Bf_uT,heading_deg");
}

void loop() {
  // Serial control
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') continue;

    if (c == 's') {
      running = false;
      Serial.println("# STOP");
    } else if (c == 'r') {
      running = true;
      Serial.println("# RUN");
    } else if (c == 'f') {
      resetFilter();
    } else if (c == 'c') {
      calibStart();
    } else if (c == 'v') {
      calibFinish();
    } else if (c == '1') {
      int16_t mx, my, mz;
      if (readMagRaw(mx, my, mz)) {
        emitSample_uT(millis(), mx, my, mz);
      } else {
        Serial.println("# Read failed");
      }
      running = false;
      Serial.println("# STOP");
    }
  }

  if (!running) return;

  // Timed sampling
  uint32_t now = millis();
  if (now - lastSampleMs < SAMPLE_PERIOD_MS) return;
  lastSampleMs = now;

  int16_t mx, my, mz;
  if (!readMagRaw(mx, my, mz)) {
    Serial.println("# Read failed");
    return;
  }

  emitSample_uT(now, mx, my, mz);
}
