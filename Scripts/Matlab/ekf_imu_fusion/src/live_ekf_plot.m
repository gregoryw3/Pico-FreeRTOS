function live_ekf_plot()
    % LIVE_EKF_PLOT - Live EKF visualization with adjustable sine wave input and noisy IMU simulation

    % Parameters
    fs = 1000;           % Sampling frequency (Hz)
    T = 20;             % Duration (s)
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
    Q = diag([0.001, 0.01]); % Try smaller or larger values
    R = 0.04;                % Try smaller or larger values

    % Create figure and sliders
    f = figure('Name', 'Live EKF Plot', 'NumberTitle', 'off', 'Position', [100 100 900 900]);
    ax1 = subplot(2,1,1, 'Parent', f); % Main plot
    ax2 = subplot(2,1,2, 'Parent', f); % Difference plot

    hTrue = plot(ax1, t, zeros(size(t)), 'b', 'LineWidth', 1.5); hold(ax1, 'on');
    % hNoisy = plot(ax1, t, zeros(size(t)), 'r', 'LineWidth', 1);
    hEKF = plot(ax1, t, zeros(size(t)), 'g', 'LineWidth', 2);
    % hMatlabEKF = plot(ax1, t, zeros(size(t)), 'm--', 'LineWidth', 2); % MATLAB EKF
    legend(ax1, 'True Signal', 'Noisy IMU', 'Your EKF', 'MATLAB EKF');
    xlabel(ax1, 'Time (s)');
    ylabel(ax1, 'Signal');
    title(ax1, 'EKF Live Plot with Adjustable Sine Input');
    grid(ax1, 'on');

    hDiffYour = plot(ax2, t, zeros(size(t)), 'g', 'LineWidth', 2); hold(ax2, 'on');
    % hDiffMatlab = plot(ax2, t, zeros(size(t)), 'm--', 'LineWidth', 2);
    legend(ax2, 'Your EKF - Truth', 'MATLAB EKF - Truth');
    xlabel(ax2, 'Time (s)');
    ylabel(ax2, 'Error');
    title(ax2, 'EKF Estimation Error vs Ground Truth');
    grid(ax2, 'on');

    % Sliders
    uicontrol('Style', 'text', 'Position', [100 70 100 20], 'String', 'Amplitude');
    sAmp = uicontrol('Style', 'slider', 'Min', 0.1, 'Max', 20, 'Value', amp, ...
        'Position', [100 50 200 20], 'Callback', @update_plot);
    ampVal = uicontrol('Style', 'text', 'Position', [310 50 40 20], 'String', num2str(amp));

    uicontrol('Style', 'text', 'Position', [350 70 100 20], 'String', 'Frequency');
    sFreq = uicontrol('Style', 'slider', 'Min', 0.1, 'Max', 20, 'Value', freq, ...
        'Position', [350 50 200 20], 'Callback', @update_plot);
    freqVal = uicontrol('Style', 'text', 'Position', [560 50 40 20], 'String', num2str(freq));

    uicontrol('Style', 'text', 'Position', [600 70 100 20], 'String', 'Phase');
    sPhase = uicontrol('Style', 'slider', 'Min', -pi, 'Max', pi, 'Value', phase, ...
        'Position', [600 50 200 20], 'Callback', @update_plot);
    phaseVal = uicontrol('Style', 'text', 'Position', [810 50 60 20], 'String', num2str(phase, '%.2f'));

    % Initial plot
    update_plot();

    function update_plot(~, ~)
        amp = get(sAmp, 'Value');
        freq = get(sFreq, 'Value');
        phase = get(sPhase, 'Value');

        % Update value labels
        set(ampVal, 'String', num2str(amp, '%.2f'));
        set(freqVal, 'String', num2str(freq, '%.2f'));
        set(phaseVal, 'String', num2str(phase, '%.2f'));

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
            omega = 2*pi*freq;
            x_pred = [x_est(1) + x_est(2)*dt;
                    x_est(2) - omega^2*x_est(1)*dt];

            % Linearize the state transition (Jacobian)
            F = [1 dt; -omega^2*dt 1];
            % F = [1 dt; 0 1];
            % x_pred = F * x_est;
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

        % --- MATLAB's EKF ---
        % Define state and measurement functions
        omega = 2*pi*freq; % Use the current frequency from the slider

        % In your update_plot function, after getting freq:
        stateFcn = @(x) [x(1) + x(2)*dt; x(2) - omega^2*x(1)*dt];
        measFcn = @(x) x(1);

        x0 = [0; 0];
        % P0 = eye(2);
        matlabEKF = extendedKalmanFilter(stateFcn, measFcn, x0, 'HasAdditiveProcessNoise', true);

        x_matlab_ekf = zeros(1, n);
        for k = 1:n
            predict(matlabEKF);
            correct(matlabEKF, imu_noisy(k));
            x_matlab_ekf(k) = matlabEKF.State(1);
        end

        % Update plots
        set(hTrue, 'YData', true_signal);
        % set(hNoisy, 'YData', imu_noisy);
        set(hEKF, 'YData', x_ekf);
        % set(hMatlabEKF, 'YData', x_matlab_ekf);

        % Plot differences in the second subplot
        set(hDiffYour, 'YData', x_ekf - true_signal);
        % set(hDiffMatlab, 'YData', x_matlab_ekf - true_signal);

        drawnow;
    end
end

function x_next = stateFcn(x, u)
    % x: [q; v; p], u: [gyro; accel]
    q = x(1:4);
    v = x(5:7);
    p = x(8:10);
    gyro = u(1:3);
    accel = u(4:6);
    dt = 0.01; % Time step

    % Quaternion update (using small-angle approx or full quaternion math)
    dq = [1; 0.5*gyro*dt]; % Small angle approx
    q_next = quatnormalize(quatmultiply(q', dq'))';

    % Velocity and position update
    g = [0; 0; -9.81];
    v_next = v + (quatrotate(q', accel')' + g) * dt;
    p_next = p + v * dt;

    x_next = [q_next; v_next; p_next];
end

function z = measFcn(x)
    % Predict IMU measurements from state
    q = x(1:4);
    v = x(5:7);
    % For example, predict accelerometer measurement (body frame)
    g = [0; 0; -9.81];
    a_world = zeros(3,1); % Example: zero acceleration in world frame
    a_body = quatrotate(quatinv(q'), (a_world - g)')';
    z = a_body;
end