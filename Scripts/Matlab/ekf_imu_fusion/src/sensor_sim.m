function [gyro_data, accel_data, true_quat, true_vel, true_pos] = sensor_sim(num_samples, noise_std, true_angles, true_accel)
    dt = 1/100;
    t = (0:num_samples-1)' * dt;

    % True quaternion (constant orientation for this example)
    true_quat = repmat([1 0 0 0], num_samples, 1);

    % True velocity and position (integrate acceleration)
    true_vel = zeros(num_samples, 3);
    true_pos = zeros(num_samples, 3);
    for k = 2:num_samples
        true_vel(k,:) = true_vel(k-1,:) + true_accel * dt;
        true_pos(k,:) = true_pos(k-1,:) + true_vel(k-1,:) * dt;
    end

    % Simulate noisy IMU data
    gyro_data = repmat(true_angles, num_samples, 1) + noise_std * randn(num_samples, 3);
    accel_data = repmat(true_accel, num_samples, 1) + noise_std * randn(num_samples, 3);
end