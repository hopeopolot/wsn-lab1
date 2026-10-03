Multi-Sensor Edge Node with Calibration

An ESP32 node reads temperature and humidity (SHT31), light (LDR) and motion (PIR). It checks every reading, applies a calibration, smooths the temperature, makes a local status decision, and prints a timestamped CSV line each second. Python notebooks turn that stream into statistics, plots and a calibration fit.


Repository structure

```
lab1/
├── firmware/
│   ├── sensor_node/
│   │   └── sensor_node.ino       
│   └── wokwi/
│       └── README.md              
├── python/            
│   └── wsn.ipynb            
├── data/
│   ├── README.md                  
│   ├── raw/
│   │   ├── sim_raw.csv            # Wokwi simulation export
│   │   ├── sample_serial_log.csv  # synthetic reference data (pipeline test only)
│   │   └── calibration_pairs.csv  # sensor vs reference thermometer
│   └── processed/                 # outputs of analysis.ipynb (CSVs, statistics, calibration_model.json)
├── figures/                       # every plot as PNG
├── docs/
│   ├── simulation-circuit_diagram.png
│   ├── system_architecture.png
│   ├── analysis_notes.md         
│   └── report_template.md        
├── requirements.txt
└── README.md
```

The layout follows the recommended structure in section 9.3 of the lab guide.

**GitHub repository: https://github.com/hopeopolot/wsn-lab1

**Wokwi simulation: https://wokwi.com/projects/476490188961995777


Firmware

`firmware/sensor_node/sensor_node.ino` runs a fixed 1 Hz loop and does the following on the ESP32:

- reads SHT31, LDR and PIR, and stamps each row with the elapsed seconds
- marks a reading invalid if it is NaN, outside the plausible range, or jumps more than 2 °C in one second (a real step change is accepted after 3 consecutive readings)
- applies the calibration `temp_cal = CAL_GAIN * temp_raw + CAL_OFFSET`
- smooths the calibrated temperature with a 5-sample moving average and reports the change between rows
- counts one motion event per rising edge of the PIR output
- sets `status` to `NORMAL`, `HIGH_TEMP`, `LOW_TEMP`, `HIGH_HUM`, `LOW_HUM` (joined with `+`) or `INVALID`, and drives the LED
- prints one CSV row per second at 115200 baud

`CAL_GAIN` is 1.0 and `CAL_OFFSET` is 0.0 until calibrated constants are copied in (see Calibration below). Invalid rows stay in the output, flagged, and are never deleted.

Required Arduino libraries: *Adafruit SHT31 Library* and *Adafruit BusIO*.

Running the simulation in Wokwi

1. Open the project link above and press **Start**.
2. Open the serial monitor. A CSV header appears, then one row per second.
3. Drive each sensor from its on-part controls: change the temperature and humidity, move the light slider, and toggle the PIR.
4. Check that a temperature above 30 °C gives `HIGH_TEMP` and lights the LED, and that `motion_event` is 1 only on the first sample of each motion.
5. Copy the serial output into a CSV and save it under `data/raw/`. The saved simulation run is `sim_raw.csv`.

The simulator has no real sensor noise, so it demonstrates the logic only.



Simulation (`sim_raw.csv`)

| Measure | Value |
|---|---|
| Rows (valid / invalid) | 812 (812 / 0) |
| Duration, effective rate | 811 s (13.5 min), 1.00 Hz |
| PIR events | 10 |
| Temperature (sliders swept) | −15.8 to 62.8 °C |
| Moving average vs independent pandas check | identical |

The temperature spread comes from moving the slider on purpose, so these numbers show the logic working, not sensor noise.



