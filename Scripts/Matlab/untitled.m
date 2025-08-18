% Define parameters
Fs = 100; % Sample rate
N = 1000; % Number of samples
Fc = 0.25; % Frequency of the sinusoidal input
t = (0:(1/Fs):((N-1)/Fs)).'; % Time vector

% Define possible input curves
input_curves = {@(t) sin(2 * pi * Fc * t), @(t) square(2 * pi * Fc * t), @(t) sawtooth(2 * pi * Fc * t)};
curve_names = {'Sine', 'Square', 'Sawtooth'};

% Default curve index
curve_idx = 1;

% Create figure for UI
fig = uifigure('Position', [100, 100, 700, 500]);

% Create a dropdown for curve selection
dd = uidropdown(fig, ...
    'Items', curve_names, ...
    'Position', [100, 120, 120, 22], ...
    'Value', curve_names{curve_idx}, ...
    'ValueChangedFcn', @(src, event) updateEKF(src, t, input_curves, curve_names, fig));

% Add a label for dropdown
uilabel(fig, 'Position', [100, 150, 120, 22], 'Text', 'Select Input Curve:');

% Axes for plotting
ax = uiaxes(fig, 'Position', [100, 200, 500, 250]);

% Run initial EKF and plot
runAndPlotEKF(ax, t, input_curves{curve_idx}, curve_names{curve_idx});

% Callback for dropdown
function updateEKF(src, t, input_curves, curve_names, fig)
    curve_idx = find(strcmp(src.Value, curve_names));
    ax = findobj(fig, 'Type', 'uiaxes');
    runAndPlotEKF(ax, t, input_curves{curve_idx}, curve_names{curve_idx});
end

% Function to run EKF and plot results
function runAndPlotEKF(ax, t, input_curve, curve_name)
    Fs = 100;
    N = numel(t);

    % Generate angular velocity data for selected curve
    angvel = zeros(N, 3);
    angvel(:, 1) = input_curve(t);

    % Add noise to the angular velocity data
    noise_std_dev = 0.1;
    noise = noise_std_dev * randn(N, 3);
    noisy_angvel = angvel + noise;

    % Prepare measurements for EKF
    measurements = cell(N, 1);
    for i = 1:N
        measurements{i} = mean(noisy_angvel(i, 1:2:end), 2)';
    end

    % EKF parameters
    initial_state = [0; 0; 0; 0];
    initial_covariance = eye(4);
    process_noise = exp(-20) * eye(4);
    measurement_noise = exp(-27) * eye(2);

    % Run custom EKF for all samples
    state_estimates = zeros(4, N);
    covariance_estimates = initial_covariance;
    state = initial_state;
    for i = 1:N
        z = measurements{i};
        [state_estimates(:, i), covariance_estimates] = EKF_step(z, state, covariance_estimates, process_noise, measurement_noise);
        state = state_estimates(:, i);
    end

    % --- MATLAB's built-in EKF (requires Sensor Fusion and Tracking Toolbox) ---
    % Define state transition and measurement functions
    f = @(x) [x(1) + x(3); x(2) + x(4); x(3); x(4)];
    h = @(x) [x(1); x(2)];
    % Jacobians
    Fjac = @(x) [1 0 1 0; 0 1 0 1; 0 0 1 0; 0 0 0 1];
    Hjac = @(x) [1 0 0 0; 0 1 0 0];

    % Create MATLAB EKF object
    try
        ekf = trackingEKF(f, h, initial_state, ...
            'StateCovariance', initial_covariance, ...
            'ProcessNoise', process_noise, ...
            'MeasurementNoise', measurement_noise, ...
            'StateTransitionJacobianFcn', Fjac, ...
            'MeasurementJacobianFcn', Hjac);

        matlab_ekf_estimates = zeros(4, N);
        for i = 1:N
            predict(ekf);
            correct(ekf, measurements{i});
            matlab_ekf_estimates(:, i) = ekf.State;
        end
    catch
        matlab_ekf_estimates = [];
    end

    % Plot results
    cla(ax);
    hold(ax, 'on');
    plot(ax, t, state_estimates(1, :), 'DisplayName', 'Custom EKF Output', 'LineWidth', 2);
    if exist('matlab_ekf_estimates', 'var') && ~isempty(matlab_ekf_estimates)
        plot(ax, t, matlab_ekf_estimates(1, :), '--', 'DisplayName', 'MATLAB EKF Output', 'LineWidth', 2);
    end
    plot(ax, t, noisy_angvel(:, 1), 'DisplayName', 'Noisy Angular Velocity', 'LineWidth', 1);
    plot(ax, t, angvel(:, 1), 'DisplayName', [curve_name ' Reference'], 'LineWidth', 2);
    xlabel(ax, 'Time (s)');
    ylabel(ax, 'State Estimate (rad/s)');
    title(ax, ['EKF Output with ' curve_name ' Input']);
    legend(ax, 'Location', 'best');
    grid(ax, 'on');
    hold(ax, 'off');
end

% EKF step function
function [state_est, covariance_est] = EKF_step(z, state, covariance, process_noise, measurement_noise)
    [state_pred, covariance_pred] = predict(state, covariance, process_noise);
    [state_est, covariance_est] = update(state_pred, covariance_pred, z, measurement_noise);
end

function [state_pred, covariance_pred] = predict(state, covariance, process_noise)
    dt = 1;
    F = [1 0 dt 0; 0 1 0 dt; 0 0 1 0; 0 0 0 1];
    state_pred = F * state;
    covariance_pred = F * covariance * F' + process_noise;
end

function [state_est, covariance_est] = update(state_pred, covariance_pred, z, measurement_noise)
    H = [1 0 0 0; 0 1 0 0];
    z_pred = H * state_pred;
    y = z - z_pred;
    S = H * covariance_pred * H' + measurement_noise;
    K = covariance_pred * H' / S;
    state_est = state_pred + K * y;
    covariance_est = (eye(size(K, 1)) - K * H) * covariance_pred;
end