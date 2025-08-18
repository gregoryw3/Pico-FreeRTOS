import rerun as rr
import numpy as np
import pandas as pd
from scipy.spatial.transform import Rotation as R
from datetime import datetime, timedelta

rr.init("Rocket Demo", spawn=True)

def parse_datetime(row):
    # Combine columns to a datetime string and parse
    dt_str = f"{int(row['Year']):04d}-{int(row['Month']):02d}-{int(row['Day']):02d}T{row['Time']}"
    return datetime.fromisoformat(dt_str)

# High-rate data
df_hr = pd.read_csv("./BlRv_SN0867 HR_06-11-2025_12_07_53.csv")
quats = df_hr[['Quat_1', 'Quat_2', 'Quat_3', 'Quat_4']].values
datetimes_hr = df_hr.apply(parse_datetime, axis=1)

# Low-rate data
df_lr = pd.read_csv("./BlRv_SN0867 LR_06-11-2025_12_07_53.csv")
alt_lr = df_lr['Baro_Altitude_AGL_(feet)'].values
datetimes_lr = df_lr.apply(parse_datetime, axis=1)

# Log gyroscope data
gyro = df_hr[["Gyro_X", "Gyro_Y", "Gyro_Z"]]
gyro_times = rr.TimeColumn("time", timestamp=datetimes_hr)
rr.send_columns("rocket/gyroscope", indexes=[gyro_times], columns=rr.Scalars.columns(scalars=gyro))

# Log accelerometer data
accel = df_hr[["Accel_X", "Accel_Y", "Accel_Z"]]
accel_times = rr.TimeColumn("time", timestamp=datetimes_hr)
rr.send_columns("rocket/accelerometer", indexes=[accel_times], columns=rr.Scalars.columns(scalars=accel))

# Log barometric altitude (AGL)
alt_agl = df_lr["Baro_Altitude_AGL_(feet)"]
alt_times = rr.TimeColumn("time", timestamp=datetimes_lr)
rr.send_columns("rocket/altitude_agl", indexes=[alt_times], columns=rr.Scalars.columns(scalars=alt_agl))

# Example: log temperature
temp = df_lr["Temperature_(F)"]
rr.send_columns("rocket/temperature", indexes=[alt_times], columns=rr.Scalars.columns(scalars=temp))

print("Logging data...")

fixed_rot = R.from_euler('y', 270, degrees=True)

for dt, alt, q in zip(datetimes_hr, np.interp(datetimes_hr.astype(np.int64), datetimes_lr.astype(np.int64), alt_lr), quats):
    rr.set_time("time", timestamp=dt)
    quat_xyzw = np.array([q[1], q[2], q[3], q[0]])
    rot = R.from_quat(quat_xyzw)
    rot_total = fixed_rot * rot
    quat_rotated = rot_total.as_quat()
    rr.log(
        "rocket/pose",
        rr.Transform3D(
            translation=[0.0, 0.0, float(alt)],
            rotation=rr.Quaternion(xyzw=quat_rotated)
        )
    )