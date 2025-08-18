function [sensorData, stateEstimates] = index()
    % Custom types and structures for EKF IMU Fusion project

    % Structure for sensor readings
    sensorData = struct();
    sensorData.gyroscopes = []; % Array to hold gyroscope readings
    sensorData.accelerometers = []; % Array to hold accelerometer readings

    % Structure for state estimates
    stateEstimates = struct();
    stateEstimates.quaternion = []; % Quaternion representing orientation
    stateEstimates.velocity = []; % Velocity vector
    stateEstimates.position = []; % Position vector

    % Covariance matrix for state estimation
    stateEstimates.covariance = eye(6); % 6x6 identity matrix for initial covariance
end