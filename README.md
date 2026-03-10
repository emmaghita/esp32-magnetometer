# ESP32 Triaxial Magnetometer - Ambient Magnetic Field Measurement

This project implements a simple embedded system for measuring and analyzing the ambient magnetic field using a triaxial magnetometer and an ESP32 microcontroller. The system acquires magnetic field data, processes it in real time, and transmits the results to a host computer for visualization and analysis.

The project was developed for the **Design with Microprocessors** course.

---

## Hardware

The experimental setup includes:

- **ESP32 DOIT DevKit V1**
- **GY-273 magnetometer module (HMC5883L compatible)**
- **USB connection to host computer**

### I²C Connections

| ESP32 | Magnetometer |
|------|-------------|
| 3.3V | VCC |
| GND | GND |
| GPIO 21 | SDA |
| GPIO 22 | SCL |

The ESP32 acts as the **I²C master** and periodically reads the magnetic field values from the sensor.

---

## Firmware

The firmware is written using the **Arduino framework for ESP32**.

Main features:

- Initialization of the magnetometer in **continuous measurement mode**
- Reading triaxial magnetic field data (X, Y, Z)
- Hard-iron calibration for offset correction
- Conversion of raw sensor counts to **microtesla (µT)**
- Noise reduction using an **Exponential Moving Average (EMA) filter**
- Magnetic heading computation
- Transmission of measurements through **USB serial (CSV format)**

---

## Data Analysis

Magnetic field measurements are recorded and analyzed using **Python** with the following libraries:

- `pandas`
- `matplotlib`

The scripts generate several plots:

- **Magnetic field magnitude (raw vs filtered)**
- **Magnetic heading over time**
- **Magnetic field components (Bx, By, Bz)**

These plots illustrate the effect of filtering and the variation of the magnetic field when the sensor orientation changes.


