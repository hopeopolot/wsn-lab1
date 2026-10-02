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

## Running the simulation in Wokwi

1. Open the project link above and press **Start**.
2. Open the serial monitor. A CSV header appears, then one row per second.
3. Drive each sensor from its on-part controls: change the temperature and humidity, move the light slider, and toggle the PIR.
4. Check that a temperature above 30 °C gives `HIGH_TEMP` and lights the LED, and that `motion_event` is 1 only on the first sample of each motion.
5. Copy the serial output into a CSV and save it under `data/raw/`. The saved simulation run is `sim_raw.csv`.

The simulator has no real sensor noise, so it demonstrates the logic only.

## Python pipeline

- **`acquisition.ipynb` / `acquisition.py`** read the serial port with pyserial and write the CSV stream to `data/raw/`. Malformed lines go to a `.rejected.txt` file, not the bin. Colab cannot see a USB port, so record on your own computer.
- **`analysis.ipynb`** loads each dataset and reports row count, duration, effective sampling rate, mean, minimum, maximum, standard deviation, invalid rows and events. It cross-checks the on-device moving average, rate of change and event count, saves seven plots per dataset, fits the calibration, estimates noise and filter lag, and compares data volumes. It runs locally or in Google Colab, which asks for the files in `data/raw/` if they are missing.

```
pip install -r requirements.txt
```

## Results so far

### Simulation (`sim_raw.csv`)

| Measure | Value |
|---|---|
| Rows (valid / invalid) | 812 (812 / 0) |
| Duration, effective rate | 811 s (13.5 min), 1.00 Hz |
| PIR events | 10 |
| Temperature (sliders swept) | −15.8 to 62.8 °C |
| Moving average vs independent pandas check | identical |

The temperature spread comes from moving the slider on purpose, so these numbers show the logic working, not sensor noise.

![Raw vs filtered, simulation](figures/sim_3_raw_vs_filtered.png)

### Calibration (`calibration_pairs.csv`)

Only two reference pairs exist, so a gain cannot be estimated and an **offset-only** correction is used: **+0.95 °C**. MAE falls from 0.95 to 0.25 °C and RMSE from 0.98 to 0.25 °C. This is a two-point result. A linear fit needs at least five pairs across the operating range. The logged data still have `temp_cal == temp_raw`, because the offset is not in the firmware yet.

![Calibration](figures/calibration_fit.png)

### Pipeline check on synthetic data

`sample_serial_log.csv` is a synthetic dataset, not a hardware recording. The notebook runs on it to test the pipeline (650 rows, 6 invalid rows, 5 events, filter noise reduction of about 64 %), but its numbers must not be reported as measurements.

### Data volume estimate

At about 60 bytes per row: raw every 1 s is 86,400 messages and 4.3 MB per day, raw every 5 s is 17,280 and 0.86 MB, and one summary per minute is 1,440 and 0.07 MB.

## What's still needed

- [ ] Record the SHT31 baseline (at least 5 minutes, at least 600 rows) with `acquisition.ipynb`, then add it to `DATASETS` in `analysis.ipynb`
- [ ] Characterization runs: LDR dark, room and bright; PIR walk-in and false triggers; temperature in two locations
- [ ] At least three more calibration pairs across the temperature range, then copy the fitted constants into the firmware and re-record
- [ ] Export the Wokwi `sketch.ino` and `diagram.json` into `firmware/wokwi/`
- [ ] Add a photo of the wired prototype and a Wokwi screenshot to `docs/`
- [ ] Write the technical report (`docs/lab_report.pdf`) from `docs/report_template.md`
- [ ] Prepare the live demonstration

The optional IMU, MQTT and buffering items from the advanced challenge are not implemented.

## Limitations

Low-cost sensors on a single node with a serial-only link. The reference thermometer's own accuracy limits how much calibration can help, and the current calibration rests on two points.
