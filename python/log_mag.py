import serial
import time

PORT = "COM12"
BAUD = 115200
DURATION_SEC = 30
OUT_FILE = "mag_ema.csv"

ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(1.0)

with open(OUT_FILE, "w", encoding="utf-8") as f:
    # write our expected header (even if ESP32 prints it too)
    f.write("t_ms,mx,my,mz,B,fx,fy,fz,Bf\n")

    start = time.time()
    while time.time() - start < DURATION_SEC:
        line = ser.readline().decode(errors="ignore").strip()
        if not line or line.startswith("#") or line.startswith("t_ms"):
            continue
        # expect 8 commas for 9 fields
        if line.count(",") != 8:
            continue
        f.write(line + "\n")

ser.close()
print("Saved:", OUT_FILE)
