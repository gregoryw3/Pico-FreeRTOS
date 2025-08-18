% filepath: c:\Users\gregw\Documents\MATLAB\ekf_imu_fusion\src\minimal_imu_ekf.m
% Minimal 3D IMU EKF example (orientation only, no velocity/position)

addpath(genpath('./quaternion_library'))

% --- Load CSV data ---
filename = 'BlRv_SN0867 HR_06-11-2025_12_07_53.csv'; % <-- Change to your CSV file path
opts = detectImportOptions(filename);
opts = setvartype(opts, {'Gyro_X','Gyro_Y','Gyro_Z','Accel_X','Accel_Y','Accel_Z','Quat_1','Quat_2','Quat_3','Quat_4'}, 'double');
data = readtable(filename, opts);

% Year,Month,Day,Time,Flight_Time_(s),Sync,Temperature_(F),Baro_Press_(atm),Baro_Altitude_ASL_(feet),Baro_Altitude_AGL_(feet),Batt_Volts,Apo_Volts,Main_Volts,3rd_Volts,4th_Volts,Velocity_Up,Velocity_DR,Velocity_CR,Inertial_Altitude,Inertial_DR_Position,Inertial_CR_position,Tilt_Angle_(deg),Future_Angle_(deg),Roll_Angle_(deg),Reserved_1,Reserved_2,Reserved_3,Rocket_FER_Hex,Apo_FER_H  ex,Main_FER_Hex,3rd_FER_Hex,4th_FER_Hex,Liftoff,Apogee,Press_Increasing,Burnout_Coast,Apo_fired,Main_fired,3rd_fired,4th_fired,Normal_Ascent,Accel_Vel_LE_0,ECI_Vvel_le_0,Tilt Exceeded 90deg,Reserved,LT_AGL1,GT_AGL2,LT_FANG1,GT_FANG2,GT_FUTANG,LT_TVAL1,GT_TVAL2,LT_VEL1,GT_VEL2,LT_BVEL1,GT_BURN,Armed,Reserved,Reserved,LT_AGL1,GT_AGL2,LT_FANG1,GT_FANG2,GT_FUTANG,LT_TVAL1,GT_TVAL2,LT_VEL1,GT_VEL2,LT_BVEL1,GT_BURN,Armed,Reserved,Reserved,LT_AGL1,GT_AGL2,LT_FANG1,GT_FANG2,GT_FUTANG,LT_TVAL1,GT_TVAL2,LT_VEL1,GT_VEL2,LT_BVEL1,GT_BURN,Armed,Reserved,Reserved,LT_AGL1,GT_AGL2,LT_FANG1,GT_FANG2,GT_FUTANG,LT_TVAL1,GT_TVAL2,LT_VEL1,GT_VEL2,LT_BVEL1,GT_BURN,Armed,Reserved,Reserved
% --- Load lower-rate Blue Raven data ---
filename_LR = 'BlRv_SN0867 LR_06-11-2025_12_07_53.csv';
opts_LR = detectImportOptions(filename_LR);
opts_LR = setvartype(opts_LR, {'Baro_Altitude_ASL__feet_', 'Velocity_Up','Velocity_DR','Velocity_CR'}, 'double');
data_LR = readtable(filename_LR, opts_LR);

filename2 = 'irec_2025_ads.csv'; % Change to your second CSV file path
opts2 = detectImportOptions(filename2);
opts2 = setvartype(opts2, {'time_us', 'altitude_filt', 'mid_imu_ax', 'mid_imu_ay', 'mid_imu_az', 'mid_imu_gx', 'mid_imu_gy', 'mid_imu_gz', 'mag_x','mag_y', 'mag_z', 'high_g_x', 'high_g_y', 'high_g_z'}, 'double');
data2 = readtable(filename2, opts2);
start_row = 11162;
end_row = 26270;
data2 = data2(start_row:end_row, :);
good_idx = [true; diff(data2.time_us) > 0];
data2 = data2(good_idx, :);

% Extract relevant columns
gyro_data = deg2rad([data.Gyro_X, data.Gyro_Y, data.Gyro_Z]); % rad/s
accel_data = [data.Accel_X, data.Accel_Y, data.Accel_Z] * 9.81; % Convert g to m/s^2, [N x 3]
csv_quat = [data.Quat_1, data.Quat_2, data.Quat_3, data.Quat_4];
N = height(data);

% Estimate dt from Flight_Time_(s) if available, else set manually
t = data.Flight_Time__s_;
dt_vec = [diff(t); median(diff(t))]; % Last dt is repeated for final sample

% Gyro: mid_imu_gx, mid_imu_gy, mid_imu_gz (assumed in deg/s)
% sensitivity = 8.2; % LSB/(deg/s) for 2000 dps range
% bias = [0, 0, 0]; % Adjust based on calibration
% gyro2 = ([data2.mid_imu_gx, data2.mid_imu_gy, data2.mid_imu_gz] - bias) / sensitivity;
gyro2 = [data2.mid_imu_gz, data2.mid_imu_gx, data2.mid_imu_gy]; % in deg/s
gyro2 = deg2rad(gyro2); % Convert to rad/s

% Accel: mid_imu_ax, mid_imu_ay, mid_imu_az (assumed in g)
accel2 = [data2.mid_imu_az, data2.mid_imu_ax, data2.mid_imu_ay] * 9.81; % in g

mag2 = [data2.mag_z, data2.mag_x, data2.mag_y]; % in uT

accel2 = [-accel2(:,3), accel2(:,2), accel2(:,1)];
gyro2  = [-gyro2(:,3), gyro2(:,2), gyro2(:,1)];
mag2   = [-mag2(:,3),  mag2(:,2),  mag2(:,1)];

% Time: use time_us (microseconds)
t2 = (data2.time_us - data2.time_us(1)) * 1e-6; % seconds, zero-based
% dt_vec2 = [diff(t2); median(diff(t2))];
N2 = height(data2);

dt_vec2 = 0.01 * ones(N2,1); % Set constant 10ms (0.01s) timestep for all samples in CSV2

%% --- Tune Madgwick Beta parameter ---
% % Try a range of beta values and pick the one minimizing error to CSV quat
% beta_vals = linspace(0.00001, 0.0001, 300); % Try 20 values from 0.001 to 0.1
% errors = zeros(size(beta_vals));
% csv_quat_norm = csv_quat ./ vecnorm(csv_quat,2,2);
% parfor b = 1:length(beta_vals) % Use parfor for speed if Parallel Toolbox is available, spawns multiple threads
%     madgwick = MadgwickAHRS('SamplePeriod', dt_vec(1), 'Beta', beta_vals(b));
%     madgwick_quat = zeros(N,4);
%     for k = 1:N
%         madgwick.SamplePeriod = dt_vec(k);
%         madgwick.UpdateIMU(gyro_data(k,:), accel_data(k,:));
%         madgwick_quat(k,:) = madgwick.Quaternion;
%     end
%     % Normalize Madgwick output
%     madgwick_quat_norm = madgwick_quat ./ vecnorm(madgwick_quat,2,2);
%     % Compute mean squared error (can use other metrics)
%     errors(b) = mean(sum((madgwick_quat_norm - csv_quat_norm).^2, 2));
% end

% % Find best beta
% [~, idx] = min(errors);
% best_beta = beta_vals(idx);
% disp(['Best Beta: ', num2str(best_beta)]);

% % Optionally, plot error vs beta
% figure;
% plot(beta_vals, errors, '-o');
% xlabel('Beta'); ylabel('Mean Squared Error'); grid on;
% title('Madgwick Beta Tuning vs CSV Quaternion');
%%

% Simulate some IMU data
% N = 1000;
% dt = 0.01;
% t = (0:N-1)*dt;
% true_angvel = repmat([0.1 0.2 0.3], N, 1); % rad/s
% true_quat = zeros(N,4);
% true_quat(1,:) = [1 0 0 0];
g = [0 0 -9.81];

% --- Full quaternion update for ground truth ---
% for k = 2:N
%     omega = true_angvel(k-1,:);
%     theta = norm(omega*dt);
%     if theta > 0
%         axis = omega / norm(omega);
%         dq = [cos(theta/2), axis*sin(theta/2)];
%     else
%         dq = [1 0 0 0];
%     end
%     true_quat(k,:) = quatnormalize(quatmultiply(true_quat(k-1,:), dq));
% end

% Simulate noisy gyro and accel
% gyro_data = true_angvel + 0.01*randn(N,3);
% accel_data = zeros(N,3);
% for k = 1:N
%     % Accelerometer measures gravity in body frame
%     accel_data(k,:) = quatrotate(quatinv(true_quat(k,:)), g);
% end
% accel_data = accel_data + 0.05*randn(N,3);

% EKF state: quaternion (4x1)
stateFcn = @(q, u, dt) quatnormalize(quatmultiply(q', axisAngle2quat(u*dt)))';
measFcn = @(q) quatrotate(quatinv(q'), g)';

% x0 = csv_quat(1,:)';
% Q_vals = logspace(-6, 0, 8); % Try Q from 1e-6 to 1
% R_vals = logspace(-6, -2, 5); % Try R from 1e-6 to 1e-2

% best_err = inf;
% best_Q = NaN;
% best_R = NaN;

% for q_idx = 1:length(Q_vals)
%     for r_idx = 1:length(R_vals)
%         Q = Q_vals(q_idx) * eye(4);
%         R = R_vals(r_idx) * eye(3);

%         % Print progress
%         fprintf('Testing Q = %.2e, R = %.2e (%d/%d, %d/%d)\n', ...
%             Q_vals(q_idx), R_vals(r_idx), q_idx, length(Q_vals), r_idx, length(R_vals));


%         ekf = extendedKalmanFilter(stateFcn, measFcn, x0, ...
%             'HasAdditiveProcessNoise', true, ...
%             'ProcessNoise', Q, ...
%             'MeasurementNoise', R);

%         est_quat_tmp = zeros(N,4);
%         for k = 1:N
%             if k == 1
%                 dt_k = dt_vec(1);
%             else
%                 dt_k = dt_vec(k-1);
%             end
%             predict(ekf, gyro_data(k,:)', dt_k);
%             correct(ekf, accel_data(k,:)');
%             est_quat_tmp(k,:) = ekf.State';
%         end

%         % Compute error to Madgwick (normalize both)
%         est_quat_tmp = est_quat_tmp ./ vecnorm(est_quat_tmp,2,2);
%         madgwick_quat_norm = madgwick_quat ./ vecnorm(madgwick_quat,2,2);
%         err = mean(sum((est_quat_tmp - madgwick_quat_norm).^2,2));

%         if err < best_err
%             best_err = err;
%             best_Q = Q_vals(q_idx);
%             best_R = R_vals(r_idx);
%         end
%     end
% end

% disp(['Best Q: ', num2str(best_Q), ', Best R: ', num2str(best_R), ', Error: ', num2str(best_err)]);

x0 = [1 0 0 0]';
P0 = 0.01 * eye(4);
Q = 2.68e-03 * eye(4); % Increase process noise
R = 0.0001 * eye(3); % Decrease measurement noise

ekf = extendedKalmanFilter(stateFcn, measFcn, x0, ...
    'HasAdditiveProcessNoise', true, ...
    'ProcessNoise', Q, ...
    'MeasurementNoise', R);

est_quat = zeros(N,4);
for k = 1:N
    if k == 1
        dt_k = dt_vec(1);
    else
        dt_k = dt_vec(k-1);
    end
    predict(ekf, gyro_data(k,:)', dt_k);
    correct(ekf, accel_data(k,:)');
    est_quat(k,:) = ekf.State';
end

madgwick_quat = zeros(N,4);
madgwick = MadgwickAHRS('SamplePeriod', dt_vec(1), 'Beta', 0.0001);
for k = 1:N
    madgwick.SamplePeriod = dt_vec(k);
    madgwick.UpdateIMU(gyro_data(k,:), accel_data(k,:));
    madgwick_quat(k,:) = madgwick.Quaternion;
end

madgwick2_quat = zeros(N2,4);
madgwick2 = MadgwickAHRS('SamplePeriod', dt_vec2(1), 'Beta', 0.0001);
for k = 1:N2
    madgwick2.SamplePeriod = dt_vec2(k);
    madgwick2.UpdateIMU(gyro2(k,:), accel2(k,:));
    madgwick2_quat(k,:) = madgwick2.Quaternion;
end

madgwick2_mag_quat = zeros(N2,4);
madgwick2_mag = MadgwickAHRS('SamplePeriod', dt_vec2(1), 'Beta', 0.0001);
for k = 1:N2
    madgwick2_mag.SamplePeriod = dt_vec2(k);
    madgwick2_mag.Update(gyro2(k,:), accel2(k,:), mag2(k,:));
    madgwick2_mag_quat(k,:) = madgwick2_mag.Quaternion;
end

% Plot estimated vs. CSV quaternions
% figure;
% subplot(4,1,1);
% % plot(t, est_quat(:,1), 'r--', 'DisplayName', 'EKF'); hold on;
% plot(t, madgwick_quat(:,1), 'g-', 'DisplayName', 'Madgwick'); hold on;
% plot(t2, madgwick2_quat(:,1), 'm-', 'DisplayName', 'Madgwick2');
% plot(t2, madgwick2_mag_quat(:,1), 'c-', 'DisplayName', 'Madgwick2+Mag');
% plot(t, csv_quat(:,1), 'b', 'LineWidth', 1.5, 'DisplayName', 'CSV');
% ylabel('w'); legend('show'); grid on; title('Quaternion Comparison');

% subplot(4,1,2);
% % plot(t, est_quat(:,2), 'r--', 'DisplayName', 'EKF'); hold on;
% plot(t, madgwick_quat(:,2), 'g-', 'DisplayName', 'Madgwick'); hold on;
% plot(t2, madgwick2_quat(:,2), 'm-', 'DisplayName', 'Madgwick2');
% plot(t2, madgwick2_mag_quat(:,2), 'c-', 'DisplayName', 'Madgwick2+Mag');
% plot(t, csv_quat(:,2), 'b', 'LineWidth', 1.5, 'DisplayName', 'CSV');
% ylabel('x'); legend('show'); grid on;

% subplot(4,1,3);
% % plot(t, est_quat(:,3), 'r--', 'DisplayName', 'EKF'); hold on;
% plot(t, madgwick_quat(:,3), 'g-', 'DisplayName', 'Madgwick'); hold on;
% plot(t2, madgwick2_quat(:,3), 'm-', 'DisplayName', 'Madgwick2');
% plot(t2, madgwick2_mag_quat(:,3), 'c-', 'DisplayName', 'Madgwick2+Mag');
% plot(t, csv_quat(:,3), 'b', 'LineWidth', 1.5, 'DisplayName', 'CSV');
% ylabel('y'); legend('show'); grid on;

% subplot(4,1,4);
% % plot(t, est_quat(:,4), 'r--', 'DisplayName', 'EKF'); hold on;
% plot(t, madgwick_quat(:,4), 'g-', 'DisplayName', 'Madgwick'); hold on;
% plot(t2, madgwick2_quat(:,4), 'm-', 'DisplayName', 'Madgwick2');
% plot(t2, madgwick2_mag_quat(:,4), 'c-', 'DisplayName', 'Madgwick2+Mag');
% plot(t, csv_quat(:,4), 'b', 'LineWidth', 1.5, 'DisplayName', 'CSV');
% ylabel('z'); xlabel('Time (s)'); legend('show'); grid on;

figure;
scale = 10; % Axis length
hold on; grid on; axis equal;
xlabel('X'); ylabel('Y'); zlabel('Z');
title('Animated Rocket Orientation (Body Axes Only)');
view(3);

hx = quiver3(0,0,0,1,0,0,0,'r','LineWidth',2);
hy = quiver3(0,0,0,0,1,0,0,'g','LineWidth',2);
hz = quiver3(0,0,0,0,0,1,0,'b','LineWidth',2);
hbody = plot3([0 1],[0 0],[0 0],'k-','LineWidth',3);
htime = text(-scale, -scale, scale, '', 'FontSize', 12, 'BackgroundColor', 'w');

xlim([-scale scale]);
ylim([-scale scale]);
zlim([-scale scale]);
legend({'Body X','Body Y','Body Z','Rocket Body'});

for i = 1:N
    q = csv_quat(i,:);
    R = quat2rotm(q); % [w x y z] format

    set(hx, 'UData', R(1,1)*scale, 'VData', R(2,1)*scale, 'WData', R(3,1)*scale);
    set(hy, 'UData', R(1,2)*scale, 'VData', R(2,2)*scale, 'WData', R(3,2)*scale);
    set(hz, 'UData', R(1,3)*scale, 'VData', R(2,3)*scale, 'WData', R(3,3)*scale);
    set(hbody, 'XData', [0 R(1,1)*scale], 'YData', [0 R(2,1)*scale], 'ZData', [0 R(3,1)*scale]);

    if exist('t','var') && length(t) >= i
        cur_time = t(i);
        set(htime, 'String', sprintf('Time: %.2f s', cur_time));
    else
        set(htime, 'String', sprintf('Frame: %d/%d', i, N));
    end

    drawnow;
end

% figure;
% subplot(2,1,1); plot(t, 'b'); title('First CSV Time');
% subplot(2,1,2); plot(t2, 'r'); title('Second CSV Time');

% disp(['First CSV median dt: ', num2str(median(diff(t)))]);
% disp(['Second CSV median dt: ', num2str(median(diff(t2)))]);

% disp(['First CSV accel range: ', num2str([min(accel_data(:)), max(accel_data(:))])]);
% disp(['Second CSV accel2 range: ', num2str([min(accel2(:)), max(accel2(:))])]);

% --- Helper function for quaternion from axis-angle ---
function dq = axisAngle2quat(omega_dt)
    theta = norm(omega_dt);
    if theta > 0
        axis = omega_dt / theta;
        axis = axis(:)'; % Ensure row vector
        dq = [cos(theta/2), axis*sin(theta/2)];
    else
        dq = [1 0 0 0];
    end
end