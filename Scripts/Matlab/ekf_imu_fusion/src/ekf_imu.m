function [quaternions, velocity, position] = ekf_imu(gyro_data, accel_data, dt)
    % EKF_IMU - Extended Kalman Filter for IMU sensor fusion
    % Inputs:
    %   gyro_data: Nx3 matrix of gyroscope readings (rad/s)
    %   accel_data: Nx3 matrix of accelerometer readings (m/s^2)
    %   dt: time step (s)
    % Outputs:
    %   quaternions: Nx4 matrix of estimated quaternions
    %   velocity: Nx3 matrix of estimated velocities (m/s)
    %   position: Nx3 matrix of estimated positions (m)

    % Number of measurements
    N = size(gyro_data, 1);
    
    % Initialize state: [quaternion; velocity; position]
    q = [1; 0; 0; 0]; % Initial quaternion (no rotation)
    v = [0; 0; 0];    % Initial velocity
    p = [0; 0; 0];    % Initial position

    % Preallocate output arrays
    quaternions = zeros(N, 4);
    velocity = zeros(N, 3);
    position = zeros(N, 3);

    % Process noise covariance
    Q = diag([0.01, 0.01, 0.01, 0.01, 0.1, 0.1, 0.1]); % Now 7x7

    % Measurement noise covariance
    R = diag([0.1, 0.1, 0.1]); % Adjust as necessary

    % State covariance
    P = eye(7); % Initial covariance

    for k = 1:N
        % Gyroscope measurement (angular velocity)
        omega = gyro_data(k, :)';

        % Accelerometer measurement
        accel = accel_data(k, :)';

        % Prediction step
        q_dot = 0.5 * quatmultiply(q', [0; omega]); % Quaternion derivative
        q = q + q_dot * dt; % Update quaternion
        q = q / norm(q); % Normalize quaternion

        % Update velocity and position
        v = v + (quatrotate(q, accel) - [0; 0; 9.81]) * dt; % Subtract gravity
        p = p + v * dt;

        % Store results
        quaternions(k, :) = q';
        velocity(k, :) = v';
        position(k, :) = p';

        % Update covariance (optional, can be expanded for full EKF)
        P = P + Q; % Process noise
    end
end

function v_rot = quatrotate(q, v)
    % Rotates vector v (3x1) by quaternion q (4x1)
    % q must be [w x y z]'
    qv = [0; v(:)];
    q_conj = [q(1); -q(2:4)];
    qv_rot = quatmultiply(quatmultiply(q, qv), q_conj);
    v_rot = qv_rot(2:4);
end

function ab = quatmultiply(a, b)
    % Quaternion multiplication: ab = a * b
    % Both a and b are 4x1: [w x y z]'
    w1 = a(1); x1 = a(2); y1 = a(3); z1 = a(4);
    w2 = b(1); x2 = b(2); y2 = b(3); z2 = b(4);
    ab = [w1*w2 - x1*x2 - y1*y2 - z1*z2;
          w1*x2 + x1*w2 + y1*z2 - z1*y2;
          w1*y2 - x1*z2 + y1*w2 + z1*x2;
          w1*z2 + x1*y2 - y1*x2 + z1*w2];
end