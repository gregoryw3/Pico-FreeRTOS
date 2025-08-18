num_samples = 1000;
noise_std = 0.1;
true_angles = [0.1 0.2 0.3];
true_accel = [0 0 9.81];
[gyro_data, accel_data, true_quat, true_vel, true_pos] = sensor_sim(num_samples, noise_std, true_angles, true_accel);

dt = 1/100;
[quaternions, velocity, position] = ekf_imu(gyro_data, accel_data, dt);


t = (0:num_samples-1) * dt;

% Define state transition and measurement functions for MATLAB's EKF
stateFcn = @(x, u) x; % Identity function (replace with your real model)
measFcn = @(x) x(1:3); % Example: just return first 3 states (replace as needed)

% Initial state and covariance
x0 = [1 0 0 0 0 0 0 0 0 0]'; % Example: [quat(4), vel(3), pos(3)]
P0 = eye(10);

% Create MATLAB EKF object
ekf = extendedKalmanFilter(stateFcn, measFcn, x0, 'HasAdditiveProcessNoise', true);

% Run MATLAB EKF over your data
matlab_quat = zeros(num_samples, 4);
matlab_vel = zeros(num_samples, 3);
matlab_pos = zeros(num_samples, 3);

for k = 1:num_samples
    % Predict and correct steps
    predict(ekf, gyro_data(k,:)'); % or your control input
    correct(ekf, accel_data(k,:)'); % or your measurement
    x_est = ekf.State;
    matlab_quat(k,:) = x_est(1:4);
    matlab_vel(k,:) = x_est(5:7);
    matlab_pos(k,:) = x_est(8:10);
end

figure;
subplot(3,1,1);
plot(t, position, 'b', t, true_pos, 'k--');
title('Estimated vs True Position');
xlabel('Time (s)');
ylabel('Position (m)');
legend('Est X','Est Y','Est Z','True X','True Y','True Z');
grid on;

% Now plot your EKF, MATLAB's EKF, and ground truth
plot(t, position(:,1), 'b', t, true_pos(:,1), 'k--', t, matlab_pos(:,1), 'r:');
legend('Your EKF','Ground Truth','MATLAB EKF');

subplot(3,1,2);
plot(t, velocity, 'b', t, true_vel, 'k--');
title('Estimated vs True Velocity');
xlabel('Time (s)');
ylabel('Velocity (m/s)');
legend('Est X','Est Y','Est Z','True X','True Y','True Z');
grid on;

subplot(3,1,3);
plot(t, quaternions, 'b', t, true_quat, 'k--');
title('Estimated vs True Quaternion');
xlabel('Time (s)');
ylabel('Quaternion Value');
legend('Est w','Est x','Est y','Est z','True w','True x','True y','True z');
grid on;