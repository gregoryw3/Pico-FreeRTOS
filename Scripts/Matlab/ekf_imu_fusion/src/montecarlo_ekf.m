amps = linspace(0.05, 20, 20);
freqs = linspace(0.05, 20, 20);
num_trials = 10; % Number of Monte Carlo runs per setting

fs = 1000;
T = 5;
t = linspace(0, T, T*fs);
n = length(t);
imu_noise_std = 0.2;

avg_err = zeros(length(amps), length(freqs));

for ia = 1:length(amps)
    for ifr = 1:length(freqs)
        err = zeros(num_trials, 1);
        for trial = 1:num_trials
            amp = amps(ia);
            freq = freqs(ifr);
            phase = 0; % or randomize if you want

            true_signal = amp * sin(2*pi*freq*t + phase);
            imu_noisy = true_signal + imu_noise_std * randn(size(t));

            % --- Your EKF (copy your EKF loop here) ---
            x_est = [0; 0];
            P = eye(2);
            Q = diag([0.001, 0.01]);
            R = 0.04;
            x_ekf = zeros(1, n);

            for k = 1:n
                dt = 1/fs;
                omega = 2*pi*freq;
                x_pred = [x_est(1) + x_est(2)*dt;
                          x_est(2) - omega^2*x_est(1)*dt];
                F = [1 dt; -omega^2*dt 1];
                P_pred = F * P * F' + Q;
                H = [1 0];
                z = imu_noisy(k);
                y = z - H * x_pred;
                S = H * P_pred * H' + R;
                K = P_pred * H' / S;
                x_est = x_pred + K * y;
                P = (eye(2) - K * H) * P_pred;
                x_ekf(k) = x_est(1);
            end

            err(trial) = mean(abs(x_ekf - true_signal));
        end
        avg_err(ia, ifr) = mean(err);
    end
end

% Visualize as a heatmap
figure;
imagesc(freqs, amps, avg_err);
set(gca, 'YDir', 'normal');
xlabel('Frequency (Hz)');
ylabel('Amplitude');
title('Average EKF Error');
colorbar;