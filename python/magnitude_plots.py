import time
import serial
import pandas as pd
import matplotlib.pyplot as plt

PORT = "COM12"
BAUD = 115200
DURATION_SEC = 30
OUT_FILE = "mag_uT.csv"

HEADER = "t_ms,bx_uT,by_uT,bz_uT,B_uT,fx_uT,fy_uT,fz_uT,Bf_uT,heading_deg\n"

ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(1.0)
ser.reset_input_buffer()

# ✅ auto-start streaming
ser.write(b"r\n")
time.sleep(0.2)

rows_written = 0

with open(OUT_FILE, "w", encoding="utf-8") as f:
    f.write(HEADER)

    start = time.time()
    while time.time() - start < DURATION_SEC:
        line = ser.readline().decode(errors="ignore").strip()

        if not line or line.startswith("#") or line.startswith("t_ms"):
            continue

        # 10 fields => 9 commas
        if line.count(",") != 9:
            continue

        f.write(line + "\n")
        rows_written += 1

ser.close()
print("Saved:", OUT_FILE)
print("Rows captured:", rows_written)

# ✅ stop gracefully if no data
if rows_written == 0:
    print("No data captured. Make sure the ESP32 is streaming and COM port is correct.")
    raise SystemExit

df = pd.read_csv(OUT_FILE)

# time in seconds since start
df["t_s"] = (df["t_ms"] - df["t_ms"].iloc[0]) / 1000.0

# Plot 1: Magnitude
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

# Plot 2: Heading
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

plt.figure()
plt.plot(df["t_s"], df["bx_uT"], label="Bx (µT)")
plt.plot(df["t_s"], df["by_uT"], label="By (µT)")
plt.plot(df["t_s"], df["bz_uT"], label="Bz (µT)")
plt.xlabel("Time (s)")
plt.ylabel("Magnetic field component (µT)")
plt.title("Triaxial magnetic field components over time")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("B_components_uT.png", dpi=300, bbox_inches="tight")
plt.savefig("B_components_uT.pdf", bbox_inches="tight")
plt.show()
