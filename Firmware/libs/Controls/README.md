# Controls, Filters, and Estimators Library

This library provides a set of different kinds of controls for working with sensors, actuators, and other hardware.

Eventually we would like to have a very diverse set of controls, including:

- **PID Controllers**: For precise control of systems.
- **Luenberger/State Observer**: For controlling systems with a linear model (commonly used in FOC motor controllers).
- **Low Pass Filters**: For smoothing out signals.
- **High Pass Filters**: For removing low-frequency noise.
- **Band Pass Filters**: For isolating a specific frequency range.
- **Notch/Band Stop Filters**: For eliminating specific frequencies.
- **Moving Average Filters**: For averaging out noise over a set of samples.
- **FIR/IIR Filters**: For implementing digital filters with different characteristics.
- **Fast Fourier Transform (FFT)**: For analyzing frequency components of signals.
- **Kalman Filters**: For estimating one dimension of a system's state
- **Extended Kalman Filters**: For non-linear systems.
  - Ideally this will have multiple implementations like one that just estimates quaternion orientation, one that estimates orientation and velocity, and one that estimates orientation, velocity, and position.
  - Depending on your use case, you may want to use a different implementation, as the more dimensions you estimate, the more computationally expensive it is.
- **Complementary Filters**: For combining different sensor data.

Some potential future controls include:

- **Model Predictive Control (MPC)**: For optimizing control actions based on a model of the system.
  - One issue with MPC is that it runs the optimization and estimation during runtime, currently we pre-compute this using Ansys (which takes several hours to run).
- **Linear Quadratic Regulator (LQR)**: For optimal control of linear systems.
- **Predictive Cause Adaptive Control (PCAC)**: For adaptive control based on predictive models.

Some Third Party Libraries will be included in this library, such as:

- **Madgwick Filter**: Outputs quaternions from an IMU.
- **Fusion Filter**: The updated version of the Madgwick filter, which is both more efficient and as a higher accuracy.
- **Mahony Filter**: An alternative to the Madgwick filter, which is also used for IMU data fusion.

## Some Notes

A good setup would probably be to have some pre filters to remove noise, pass that to Madgwick or Fusion, and then pass that and mabye the raw and/or filtered data to an Extended Kalman Filter (EKF) to fuse orientation, velocity, and position.

EKF requires a good starting orientation, so you must use a complementary filter or Madgwick/Fusion to get a good initial orientation.

At high velocities, GPS altitude is unusable, so might be better to just use a barometer for altitude and ignore GPS altitude.

## Dependencies

- **Eigen**: A C++ template library for linear algebra.

## Additional Reading

- <https://ahrs.readthedocs.io/en/latest/>
- <https://www.vectornav.com/resources/inertial-navigation-primer/math-fundamentals/math-filtering>