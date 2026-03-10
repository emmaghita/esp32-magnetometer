import time
import serial
import pandas as pd
import matplotlib.pyplot as plt

# ----------------------
# Serial logging settings
# ----------------------
PORT = "COM12"
BAUD = 115200
DURATION_SEC = 30
OUT_FILE = "mag_uT.csv"

# CSV header expected from the ESP32 (microtesla-only firmware)
HEADER = "t_ms,bx_uT,by_uT,bz_uT,B_uT,fx_uT,fy_uT,fz_uT,Bf_uT,heading_deg\n"

# ----------------------
# 1) Log data from ESP32
# ----------------------
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(1.0)
ser.reset_input_buffer()

with open(OUT_FILE, "w", encoding="utf-8") as f:
    f.write(HEADER)

    start = time.time()
    while time.time() - start < DURATION_SEC:
        line = ser.readline().decode(errors="ignore").strip()

        # Skip empty/comment/header lines
        if not line or line.startswith("#") or line.startswith("t_ms"):
            continue

        # Expect 10 fields => 9 commas
        if line.count(",") != 9:
            continue

        f.write(line + "\n")

ser.close()
print("Saved:", OUT_FILE)

# ----------------------
# 2) Load CSV for plotting
# ----------------------
df = pd.read_csv(OUT_FILE)

# Time in seconds since start (nicer plots)
df["t_s"] = (df["t_ms"] - df["t_ms"].iloc[0]) / 1000.0

# -----------------------------------------
# Plot 1: Magnetic field magnitude (µT)
# -----------------------------------------
plt.figure()
plt.plot(df["t_s"], df["B_uT"], label="|B| raw (µT)")
plt.plot(df["t_s"], df["Bf_uT"], label="|B| filtered (EMA)")
plt.xlabel("Time (s)")
plt.ylabel("Magnetic field magnitude (µT)")
plt.title("Magnetic field magnitude: raw vs EMA filtered")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("B_magnitude_uT.png", dpi=300, bbox_inches="tight")
plt.savefig("B_magnitude_uT.pdf", bbox_inches="tight")
plt.show()

# -----------------------------------------
# Plot 2: Heading vs time (degrees)
# -----------------------------------------
plt.figure()
plt.plot(df["t_s"], df["heading_deg"])
plt.xlabel("Time (s)")
plt.ylabel("Heading (deg)")
plt.title("Magnetic heading over time")
plt.grid(True)
plt.tight_layout()
plt.savefig("heading_vs_time.png", dpi=300, bbox_inches="tight")
plt.savefig("heading_vs_time.pdf", bbox_inches="tight")
plt.show()
