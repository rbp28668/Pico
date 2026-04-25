# QMI8658A: Sensor Reading, Quaternion Attitude Estimation, and GPS-Aided Tracking

## Part 1: Reading Acceleration and Angular Rate from the QMI8658A

### Overview

The QMI8658A is a 6-axis MEMS IMU containing a 3-axis accelerometer and a 3-axis gyroscope. It uses a right-handed coordinate system. Positive acceleration is along the positive axis direction. Positive angular rate is counterclockwise around the respective axis (right-hand rule).

Key sensor specifications:

| Parameter | Accelerometer | Gyroscope |
|-----------|--------------|-----------|
| Noise density | 150 ug/sqrt(Hz) | 13 mdps/sqrt(Hz) |
| Full-scale range | +/-2g to +/-16g | +/-16 dps to +/-2048 dps |
| Initial offset tolerance | +/-100 mg | +/-10 dps |
| Offset TCO | +/-1 mg/degC | X/Y: +/-0.1 dps/degC, Z: +/-0.05 dps/degC |
| Sensitivity TCS | +/-0.04 %/degC | X/Y: +/-0.05 %/degC, Z: +/-0.01 %/degC |
| Cross-axis sensitivity | +/-1 % | +/-2 % |

### Hardware Interface

The QMI8658A communicates via I2C (up to 400 kHz), I3C (up to 12.5 MHz SDR), or SPI (up to 15 MHz, 3-wire or 4-wire). The I2C slave address is 0x6A (SA0 high or float) or 0x6B (SA0 low). It requires two 100 nF decoupling caps on VDD and VDDIO (both 1.8 V typical, range 1.71-3.6 V).

### Initialization Sequence

After power-on, wait approximately 15 ms for the internal initialization to complete. Then configure registers as follows.

#### Step 1: Verify Chip Identity

```
Read WHO_AM_I (0x00) -> expect 0x05
Read REVISION_ID (0x01) -> expect 0x7C
```

#### Step 2: Configure the Accelerometer (CTRL2, Register 0x03)

| Bits | Field | Purpose |
|------|-------|---------|
| 7 | aST | Self-test enable (0 = off) |
| 6:4 | aFS | Full-scale: 000=+/-2g, 001=+/-4g, 010=+/-8g, 011=+/-16g |
| 3:0 | aODR | Output data rate |

Accelerometer ODR settings (accel-only mode / 6DOF mode):

| Setting | Accel-only (Hz) | 6DOF (Hz) | Mode |
|---------|----------------|-----------|------|
| 0000 | N/A | 7174.4 | Normal |
| 0001 | N/A | 3587.2 | Normal |
| 0010 | N/A | 1793.6 | Normal |
| 0011 | 1000 | 896.8 | Normal |
| 0100 | 500 | 448.4 | Normal |
| 0101 | 250 | 224.2 | Normal |
| 0110 | 125 | 112.1 | Normal |
| 0111 | 62.5 | 56.05 | Normal |
| 1000 | 31.25 | 28.025 | Normal |
| 1100 | 128 | N/A | Low Power |
| 1101 | 21 | N/A | Low Power |
| 1110 | 11 | N/A | Low Power |
| 1111 | 3 | N/A | Low Power |

Example: +/-8g at 896.8 Hz (6DOF mode) -> write `0x23` to register 0x03.

#### Step 3: Configure the Gyroscope (CTRL3, Register 0x04)

| Bits | Field | Purpose |
|------|-------|---------|
| 7 | gST | Self-test enable (0 = off) |
| 6:4 | gFS | Full-scale: 000=+/-16dps, 001=+/-32dps, ... 111=+/-2048dps |
| 3:0 | gODR | Output data rate |

Gyroscope ODR settings (all 6DOF-synchronized):

| Setting | ODR (Hz) | Mode |
|---------|----------|------|
| 0000 | 7174.4 | Normal |
| 0001 | 3587.2 | Normal |
| 0010 | 1793.6 | Normal |
| 0011 | 896.8 | Normal |
| 0100 | 448.4 | Normal |
| 0101 | 224.2 | Normal |
| 0110 | 112.1 | Normal |
| 0111 | 56.05 | Normal |
| 1000 | 28.025 | Normal |

Example: +/-2048 dps at 896.8 Hz -> write `0x73` to register 0x04.

In 6DOF mode (both accel and gyro enabled), all ODR frequencies are derived from and synchronized to the gyroscope's 22.42 kHz natural frequency. The accelerometer low-power mode is only available when the gyroscope is disabled.

#### Step 4: Configure Low-Pass Filters (CTRL5, Register 0x06) — Optional

| Bits | Field | Purpose |
|------|-------|---------|
| 6:5 | gLPF_MODE | Gyro LPF bandwidth mode |
| 4 | gLPF_EN | Enable gyroscope low-pass filter |
| 2:1 | aLPF_MODE | Accel LPF bandwidth mode |
| 0 | aLPF_EN | Enable accelerometer low-pass filter |

LPF bandwidth modes (same for both accel and gyro):

| Mode | Bandwidth |
|------|-----------|
| 00 | 2.66% of ODR |
| 01 | 3.63% of ODR |
| 10 | 5.39% of ODR |
| 11 | 13.37% of ODR |

For fusion applications, enabling the LPF at mode 10 or 11 suppresses high-frequency vibration noise before it enters the filter.

#### Step 5: Enable the Sensors (CTRL7, Register 0x08)

| Bits | Field | Purpose |
|------|-------|---------|
| 7 | SyncSample | Enable locking mechanism for coherent reads |
| 5 | DRDY_DIS | 0: DRDY driven to INT2, 1: DRDY blocked |
| 4 | gSN | 0: Gyro full mode, 1: Gyro snooze mode |
| 1 | gEN | Enable gyroscope |
| 0 | aEN | Enable accelerometer |

Write `0x83` to enable both sensors in SyncSample mode (coherent sample locking).

### Reading Sensor Data

The output data registers are at consecutive addresses, readable in a single burst read:

| Register | Address | Content |
|----------|---------|---------|
| TEMP_L | 0x33 | Temperature low byte |
| TEMP_H | 0x34 | Temperature high byte |
| AX_L | 0x35 | X acceleration low byte |
| AX_H | 0x36 | X acceleration high byte |
| AY_L | 0x37 | Y acceleration low byte |
| AY_H | 0x38 | Y acceleration high byte |
| AZ_L | 0x39 | Z acceleration low byte |
| AZ_H | 0x3A | Z acceleration high byte |
| GX_L | 0x3B | X angular rate low byte |
| GX_H | 0x3C | X angular rate high byte |
| GY_L | 0x3D | Y angular rate low byte |
| GY_H | 0x3E | Y angular rate high byte |
| GZ_L | 0x3F | Z angular rate low byte |
| GZ_H | 0x40 | Z angular rate high byte |

All values are 16-bit two's complement, little-endian (low byte at lower address by default).

#### Read Procedure (SyncSample Mode)

1. Poll `STATUSINT` (0x2D) until bit 0 (Avail) = 1
2. Burst-read 14 bytes from 0x33 through 0x40
3. The locking mechanism ensures all 6 axes plus temperature are from the same sample

The 3-byte TIMESTAMP registers (0x30-0x32) increment by 1 per sample and wrap at 0xFFFFFF. These should be used for precise `dt` computation rather than relying on host timer jitter.

### Converting Raw Values to Physical Units

#### Acceleration

```
accel_g = raw_value / sensitivity
accel_m_s2 = accel_g * 9.80665
```

| Full-Scale | Sensitivity (LSB/g) |
|------------|-------------------|
| +/-2g | 16384 |
| +/-4g | 8192 |
| +/-8g | 4096 |
| +/-16g | 2048 |

#### Angular Rate

```
rate_dps = raw_value / sensitivity
rate_rad_s = rate_dps * (pi / 180)
```

| Full-Scale | Sensitivity (LSB/dps) |
|------------|---------------------|
| +/-16 dps | 2048 |
| +/-32 dps | 1024 |
| +/-64 dps | 512 |
| +/-128 dps | 256 |
| +/-256 dps | 128 |
| +/-512 dps | 64 |
| +/-1024 dps | 32 |
| +/-2048 dps | 16 |

#### Temperature

```
T_celsius = TEMP_H + (TEMP_L / 256)
```

Output is two's complement with 1/256 degC per LSB resolution. The internal temperature sensor refreshes at 8 Hz.

---

## Part 2: Quaternion-Based Attitude Estimation via Sensor Fusion

### Why Quaternions

A 6-axis IMU cannot directly output orientation. It must be computed by fusing the accelerometer (gravity reference, low-frequency) with the gyroscope (angular rate, high-frequency but drifts over time).

Euler angles (roll, pitch, yaw) suffer from **gimbal lock**: at +/-90 degrees pitch, roll and yaw become indistinguishable and the system loses a degree of freedom. Even near +/-90 degrees the math becomes ill-conditioned. For any device that can pitch steeply, this is an operational failure, not a theoretical concern.

Quaternion advantages:

- No singularities at any orientation
- Smooth interpolation (SLERP) between attitudes
- Composition is a single quaternion multiply (16 multiplies + 12 adds) vs. repeated trig calls
- Normalization (divide by magnitude) is cheaper and more stable than re-orthogonalizing a rotation matrix
- Storage: 4 floats vs. 9 for a rotation matrix

### Quaternion Fundamentals

A unit quaternion `q = [w, x, y, z]` where `w^2 + x^2 + y^2 + z^2 = 1` represents a rotation of angle theta around unit axis `[ux, uy, uz]`:

```
q = [cos(theta/2), sin(theta/2)*ux, sin(theta/2)*uy, sin(theta/2)*uz]
```

Identity (no rotation) is `q = [1, 0, 0, 0]`.

#### Quaternion Multiply

Composition of rotations (apply `p` then `q`):

```
r = q (x) p

r.w = q.w*p.w - q.x*p.x - q.y*p.y - q.z*p.z
r.x = q.w*p.x + q.x*p.w + q.y*p.z - q.z*p.y
r.y = q.w*p.y - q.x*p.z + q.y*p.w + q.z*p.x
r.z = q.w*p.z + q.x*p.y - q.y*p.x + q.z*p.w
```

#### Rotate a Vector

To rotate vector `v = [vx, vy, vz]` by quaternion `q`:

```
v_rotated = q (x) [0, vx, vy, vz] (x) q*
```

where `q* = [w, -x, -y, -z]` is the conjugate. Efficient expansion (18 multiplies):

```
t = 2 * cross([q.x, q.y, q.z], v)
v_rotated = v + q.w * t + cross([q.x, q.y, q.z], t)
```

#### Quaternion to Rotation Matrix

When needed for EKF Jacobians or rotating vectors in bulk:

```
R = | 1-2(y^2+z^2)   2(xy-wz)       2(xz+wy)     |
    | 2(xy+wz)       1-2(x^2+z^2)   2(yz-wx)      |
    | 2(xz-wy)       2(yz+wx)       1-2(x^2+y^2)  |
```

#### Extract Euler Angles (Output Stage Only)

Euler angles should only be extracted for display or for control laws that expect them. They should never be used as internal state:

```
roll  = atan2(2(wy + xz), 1 - 2(y^2 + z^2))
pitch = asin(clamp(2(wx - yz), -1, 1))
yaw   = atan2(2(wz + xy), 1 - 2(x^2 + z^2))
```

### Filter Options

#### Complementary Filter (Simplest)

Blends gyroscope integration (high-pass) with accelerometer tilt (low-pass) using a single tuning parameter alpha (typically 0.96-0.98):

```
roll  = alpha * (roll  + gx * dt) + (1 - alpha) * roll_accel
pitch = alpha * (pitch + gy * dt) + (1 - alpha) * pitch_accel
yaw   = yaw + gz * dt   // no accelerometer correction for yaw
```

Simple to implement but provides no bias estimation and has limited tuning flexibility.

#### Madgwick / Mahony Filters (Good Middle Ground)

These are gradient-descent-based orientation filters designed specifically for IMUs. They maintain a quaternion and run in O(1) per sample, making them suitable for microcontrollers:

- **Mahony filter:** Proportional-integral (PI) controller corrects gyro integration with accelerometer reference. Two tuning parameters (Kp, Ki).
- **Madgwick filter:** Gradient descent minimizes error between measured and expected sensor readings. One tuning parameter (beta).

#### Multiplicative Extended Kalman Filter (MEKF) — Recommended

The MEKF is the standard approach used in aerospace and robotics (PX4, ArduPilot, etc.). It provides optimal fusion, automatic bias estimation, and principled uncertainty tracking.

### MEKF Design for the QMI8658A

#### Error-State Formulation

Rather than filtering the quaternion directly, the MEKF maintains the quaternion as a "nominal" state and filters a small error rotation vector. This avoids the unit-norm constraint inside the filter and keeps the covariance matrix properly sized:

**Nominal state:**
```
q     = attitude quaternion (4 components, maintained outside the filter)
bg    = gyroscope bias estimate (3 components)
```

**Error state (what the Kalman filter actually estimates):**
```
dx = [dtheta_x, dtheta_y, dtheta_z, dbg_x, dbg_y, dbg_z]   // 6 states
```

#### Prediction Step (Every Gyro Sample)

At each IMU sample (e.g., 448.4 Hz):

1. **Correct raw gyro for estimated bias:**
   ```
   omega = omega_raw - bg   (in rad/s)
   ```

2. **Integrate quaternion via exact axis-angle rotation:**
   ```
   theta = |omega| * dt
   if theta > 0:
       axis = omega / |omega|
       dq = [cos(theta/2), sin(theta/2) * axis]
       q = q (x) dq
       normalize(q)
   ```
   For small theta (< 0.01 rad), use Taylor approximation: `sin(theta/2) ~ theta/2`, `cos(theta/2) ~ 1`.

3. **Propagate error-state covariance:**
   ```
   F = | -[omega_x]  -I |      (6x6 state transition Jacobian)
       |  0           0 |

   where [omega_x] is the skew-symmetric matrix of omega

   P = F*P*F' + Q
   ```

#### Update Step (Every Accel Sample)

The accelerometer measures gravity plus dynamic acceleration. In the sensor body frame, the expected gravity given attitude `q` is:

```
g_expected = rotate(q*, [0, 0, g])     // nav-frame gravity rotated to body frame
g_measured = [ax, ay, az]              // raw accelerometer reading
innovation = g_measured - g_expected
```

Compute the measurement Jacobian H (3x6), Kalman gain K, and apply the correction:

```
S = H * P * H' + R
K = P * H' * S^(-1)
dx = K * innovation
```

Apply the correction to the nominal state:

```
// Quaternion correction (small-angle approximation):
dq = [1, dx[0]/2, dx[1]/2, dx[2]/2]
q = q (x) dq
normalize(q)

// Bias correction:
bg = bg + dx[3:5]

// Reset error state and update covariance:
P = (I - K*H) * P
```

The accelerometer correction naturally has no yaw component because gravity is vertical: rotating around the vertical axis does not change the gravity vector. This means yaw will drift with gyro-only integration until GPS or a magnetometer provides a heading reference.

#### Process Noise from QMI8658A Specifications

**Gyro noise density:** 13 mdps/sqrt(Hz) = 2.27e-4 rad/s/sqrt(Hz)

```
sigma_gyro = 2.27e-4 * sqrt(ODR)  rad/s
// At 448.4 Hz: sigma_gyro ~ 0.0048 rad/s per sample
Q_gyro = sigma_gyro^2 * dt  (continuous-to-discrete conversion)
```

**Accel noise density:** 150 ug/sqrt(Hz) = 1.47e-3 m/s^2/sqrt(Hz)

```
sigma_accel = 1.47e-3 * sqrt(ODR)  m/s^2
// At 448.4 Hz: sigma_accel ~ 0.031 m/s^2 per sample
R_accel = sigma_accel^2 * I(3x3)
```

**Gyro bias random walk** (not directly stated in the datasheet, inferred from TCO):

```
Q_bias ~ (1e-5)^2 * dt  rad^2/s^3
```

Tune empirically: larger values make bias tracking faster but noisier; smaller values give smoother bias estimates but slower adaptation.

#### Startup Calibration

At startup, hold the device stationary for 1-2 seconds:

- The EKF converges on roll and pitch from the gravity vector within a few hundred milliseconds.
- Gyro bias estimates converge within 1-2 seconds.
- Run the QMI8658A's Calibration-On-Demand (COD) via CTRL9 command 0xA2 to recalibrate gyro sensitivity. The gyro must be disabled during COD. Check COD_STATUS (0x46) for success.
- Optionally apply manual gyro offset correction via CTRL_CMD_GYRO_HOST_DELTA_OFFSET (CTRL9 command 0x0A) using CAL registers. Format is signed 11.5 fixed-point (5 fraction bits, unit 1/2^5). This offset is lost on power cycle or reset.

---

## Part 3: GPS/INS Fusion for Track Information

### Why Fuse GPS and IMU

| Property | GPS | IMU |
|----------|-----|-----|
| Position | Absolute, ~2-5m accuracy | Drifts (double-integrated accel) |
| Velocity | Good (~0.1 m/s) | Drifts |
| Orientation | Heading only (from velocity, >~5 km/h) | Excellent roll/pitch, yaw drifts |
| Update rate | Slow (1-10 Hz) | Fast (100-1000 Hz) |
| Outage tolerance | Fails indoors/tunnels/jamming | Always available |

GPS bounds IMU drift; the IMU fills GPS gaps and provides high-rate updates between GPS fixes.

### Loosely-Coupled GPS/INS EKF Architecture

This extends the attitude-only MEKF from Part 2 to include position, velocity, and accelerometer bias.

#### Full State Vector (16 Nominal States)

```
Nominal state:
  q   = [q0, q1, q2, q3]     // attitude quaternion (4)
  p   = [px, py, pz]          // position in NED navigation frame (3)
  v   = [vx, vy, vz]          // velocity in NED navigation frame (3)
  bg  = [bgx, bgy, bgz]       // gyroscope bias (3)
  ba  = [bax, bay, baz]       // accelerometer bias (3)
```

The navigation frame is NED (North-East-Down) with origin at a chosen reference point (e.g., first GPS fix).

#### Error State Vector (15 States)

```
dx = [dtheta(3), dp(3), dv(3), dbg(3), dba(3)]
```

The 15x15 covariance matrix P tracks uncertainty in all error states.

#### IMU Prediction (High Rate, Every IMU Sample)

1. **Remove estimated biases from raw measurements:**
   ```
   omega = omega_raw - bg      (rad/s, body frame)
   a     = a_raw - ba          (m/s^2, body frame)
   ```

2. **Update quaternion from corrected angular rate:**
   ```
   theta = |omega| * dt
   dq = [cos(theta/2), sin(theta/2) * omega/|omega|]
   q = q (x) dq
   normalize(q)
   ```

3. **Rotate corrected acceleration into navigation frame and subtract gravity:**
   ```
   a_nav = R(q) * a - [0, 0, g]
   ```
   where `g = 9.80665 m/s^2` and in NED, gravity is `[0, 0, +g]`.

4. **Integrate velocity and position:**
   ```
   v_new = v + a_nav * dt
   p_new = p + v * dt + 0.5 * a_nav * dt^2
   ```

5. **Propagate error-state covariance:**
   ```
   F = 15x15 state transition Jacobian (function of q, a, omega)
   P = F * P * F' + Q
   ```

#### GPS Update (Low Rate, Every GPS Fix)

When a GPS fix arrives (typically 1-10 Hz), compute innovations:

```
innovation_pos = GPS_position_NED - p
innovation_vel = GPS_velocity_NED - v
```

Convert GPS latitude/longitude/altitude to the local NED frame relative to the reference point. The measurement noise R is set from the GPS receiver's reported accuracy (HDOP, VDOP, velocity accuracy).

Compute Kalman gain and apply corrections to all 15 error states:

```
H = measurement Jacobian (6x15 for pos+vel update)
S = H * P * H' + R_gps
K = P * H' * S^(-1)
dx = K * [innovation_pos; innovation_vel]
```

Apply corrections:

```
dq = [1, dx[0]/2, dx[1]/2, dx[2]/2]
q = q (x) dq
normalize(q)

p  = p  + dx[3:5]
v  = v  + dx[6:8]
bg = bg + dx[9:11]
ba = ba + dx[12:14]

P = (I - K*H) * P
dx = 0   // reset error state
```

The GPS velocity observation also provides yaw observability when the vehicle is moving, bounding the gyro-integrated heading drift that the accelerometer alone cannot correct.

### Producing Track Information

With the fused state, the system outputs:

- **Position:** Convert NED back to latitude/longitude/altitude using the reference point.
- **Velocity:** Speed over ground = `sqrt(vx^2 + vy^2)`, vertical speed = `vz`.
- **Heading/Course:** From velocity vector `atan2(vy, vx)` when moving; from quaternion yaw when stationary (with accumulated drift caveat).
- **Attitude:** Roll, pitch, yaw extracted from quaternion at full IMU rate (e.g., 448.4 Hz).
- **Track:** Time-stamped sequence of position/velocity/attitude logged at the desired output rate.

### Handling GPS Outages

During a GPS outage (tunnel, urban canyon, jamming, interference):

- The EKF continues predicting from IMU alone (dead reckoning).
- Position accuracy degrades over time, roughly proportional to `ba * t^2` for position.
- With the QMI8658A's accel noise of 150 ug/sqrt(Hz) and initial offset of +/-100 mg, free-inertial position drift is on the order of approximately 50 m after 30 seconds without corrections.
- When GPS returns, the filter reconverges quickly because it maintained a valid orientation and bias estimate throughout the outage.

### Zero-Velocity Updates (ZUPT)

The QMI8658A's built-in motion detection engines (Any-Motion, No-Motion, configured via CTRL8 and CTRL9 commands) can detect when the device is stationary. When No-Motion is detected:

```
innovation = [0, 0, 0] - v    // velocity should be zero
```

Apply as a measurement update with tight noise. This dramatically reduces position drift during stops and helps the filter converge on accelerometer bias.

### Practical Implementation Notes

1. **Time synchronization:** The GPS PPS (pulse-per-second) signal should align IMU sample timestamps with GPS time. Misalignment of even a few milliseconds causes velocity-dependent position errors.

2. **Lever arm correction:** If the GPS antenna and IMU are not co-located, compensate the offset using the attitude estimate:
   ```
   GPS_at_IMU = GPS_pos - R(q) * lever_arm
   ```

3. **Initial alignment:** At startup, hold stationary for 1-2 seconds. The EKF estimates gravity direction (roll/pitch) and gyro biases. Initial heading comes from the first GPS velocity vector when the device starts moving.

4. **QMI8658A-specific recommendations:**
   - Use 6DOF mode at 224-449 Hz for a good balance of accuracy and processing load.
   - Enable the on-chip LPF (CTRL5) at mode 10 or 11 (5.39% or 13.37% of ODR) to suppress vibration.
   - Use the FIFO (up to 128 samples deep, 1536 bytes) to buffer IMU data so the host processor can handle GPS processing without missing IMU samples. Configure via FIFO_CTRL (0x14) and read via the CTRL9 FIFO protocol.
   - Periodically run COD to recalibrate gyroscope sensitivity, especially after temperature changes.

---

## Complete Data Flow Summary

```
QMI8658A Registers              Quaternion MEKF                 Outputs
------------------              ---------------                 -------

GX/GY/GZ (0x3B-0x40)  ------>  Predict:                ------> q (attitude quaternion)
  (angular rate, dps)           - Bias-correct gyro             |
                                - Quaternion integrate          +--> roll, pitch (display)
                                - Propagate covariance          |
                                                                |
AX/AY/AZ (0x35-0x3A)  ------>  Update (gravity ref):   ------> roll, pitch correction
  (acceleration, g)             - Expected vs measured          |    (yaw unobservable
                                - Kalman gain                   |     from accel alone)
                                - Correct q and biases          |
                                                                |
GPS (NMEA/UBX)         ------>  Update (pos + vel):     ------> position (lat/lon/alt)
  (1-10 Hz)                     - Position innovation           velocity (speed/course)
                                - Velocity innovation           heading (bounded yaw)
                                - Correct all states            track (logged positions)

TEMP_L/H (0x33-0x34)  ------>  Temperature compensation
  (die temperature)             of bias and sensitivity
                                models

No-Motion detect       ------>  ZUPT update:            ------> Reduced drift at stops
  (CTRL8/STATUS1)               - Zero-velocity
                                  measurement
```

The quaternion is the single source of truth for attitude throughout the pipeline. Euler angles are only extracted at the output stage for human display or for control laws that expect them.
