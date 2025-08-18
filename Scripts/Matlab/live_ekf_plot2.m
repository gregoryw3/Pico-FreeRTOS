function live_ekf_plot2()
    % LIVE_EKF_PLOT - Live EKF visualization with adjustable sine wave input and noisy IMU simulation

    % Parameters
    fs = 100;           % Sampling frequency (Hz)
    T = 10;             % Duration (s)
    t = linspace(0, T, T*fs);
    n = length(t);

    % Initial sine wave parameters
    amp = 1;
    freq = 1;
    phase = 0;

    % Noise parameters
    imu_noise_std = 0.2;

    % EKF parameters
    x_est = [0; 0];     % Initial state: [position; velocity]
    P = eye(2);         % Initial covariance
    Q = 0.01*eye(2);    % Process noise covariance
    R = imu_noise_std^2;% Measurement noise covariance

    % Create figure and sliders
    f = figure('Name', 'Live EKF Plot', 'NumberTitle', 'off', 'Position', [100 100 900 600]);
    ax = axes('Parent', f, 'Position', [0.08 0.3 0.88 0.65]);
    hTrue = plot(ax, t, zeros(size(t)), 'b', 'LineWidth', 1.5); hold on;
    hNoisy = plot(ax, t, zeros(size(t)), 'r.', 'MarkerSize', 6);
    hEKF = plot(ax, t, zeros(size(t)), 'g', 'LineWidth', 2);
    legend('True Signal', 'Noisy IMU', 'EKF Estimate');
    xlabel('Time (s)');
    ylabel('Signal');
    title('EKF Live Plot with Adjustable Sine Input');
    grid on;

    % Sliders
    uicontrol('Style', 'text', 'Position', [100 70 100 20], 'String', 'Amplitude');
    sAmp = uicontrol('Style', 'slider', 'Min', 0.1, 'Max', 2, 'Value', amp, ...
        'Position', [100 50 200 20], 'Callback', @update_plot);

    uicontrol('Style', 'text', 'Position', [350 70 100 20], 'String', 'Frequency');
    sFreq = uicontrol('Style', 'slider', 'Min', 0.1, 'Max', 5, 'Value', freq, ...
        'Position', [350 50 200 20], 'Callback', @update_plot);

    uicontrol('Style', 'text', 'Position', [600 70 100 20], 'String', 'Phase');
    sPhase = uicontrol('Style', 'slider', 'Min', -pi, 'Max', pi, 'Value', phase, ...
        'Position', [600 50 200 20], 'Callback', @update_plot);

    % Initial plot
    update_plot();

    function update_plot(~, ~)
        amp = get(sAmp, 'Value');
        freq = get(sFreq, 'Value');
        phase = get(sPhase, 'Value');

        % True signal (sine wave)
        true_signal = amp * sin(2*pi*freq*t + phase);

        % Simulate noisy IMU data
        imu_noisy = true_signal + imu_noise_std * randn(size(t));

        % EKF implementation for 1D position tracking
        % State: [position; velocity]
        % Measurement: position (noisy)
        x_est = [0; 0];
        P = eye(2);
        x_ekf = zeros(1, n);

        for k = 1:n
            dt = 1/fs;
            % State transition
            F = [1 dt; 0 1];
            x_pred = F * x_est;
            P_pred = F * P * F' + Q;

            % Measurement update
            H = [1 0];
            z = imu_noisy(k);
            y = z - H * x_pred;
            S = H * P_pred * H' + R;
            K = P_pred * H' / S;
            x_est = x_pred + K * y;
            P = (eye(2) - K * H) * P_pred;

            x_ekf(k) = x_est(1);
        end

        % Update plots
        set(hTrue, 'YData', true_signal);
        set(hNoisy, 'YData', imu_noisy);
        set(hEKF, 'YData', x_ekf);
        drawnow;
    end
end