function q = quat_multiply(q1, q2)
    % Quaternion multiplication
    q = [q1(1)*q2(1) - q1(2)*q2(2) - q1(3)*q2(3) - q1(4)*q2(4);
         q1(1)*q2(2) + q1(2)*q2(1) + q1(3)*q2(4) - q1(4)*q2(3);
         q1(1)*q2(3) - q1(2)*q2(4) + q1(3)*q2(1) + q1(4)*q2(2);
         q1(1)*q2(4) + q1(2)*q2(3) - q1(3)*q2(2) + q1(4)*q2(1)];
end

function q = quat_normalize(q)
    % Normalize a quaternion
    norm_q = norm(q);
    if norm_q > 0
        q = q / norm_q;
    else
        q = [1; 0; 0; 0]; % Return identity quaternion if norm is zero
    end
end

function euler = quat_to_euler(q)
    % Convert quaternion to Euler angles (roll, pitch, yaw)
    euler(1) = atan2(2*(q(1)*q(2) + q(3)*q(4)), 1 - 2*(q(2)^2 + q(3)^2)); % Roll
    euler(2) = asin(2*(q(1)*q(3) - q(4)*q(2))); % Pitch
    euler(3) = atan2(2*(q(1)*q(4) + q(2)*q(3)), 1 - 2*(q(3)^2 + q(4)^2)); % Yaw
end

function q = euler_to_quat(euler)
    % Convert Euler angles (roll, pitch, yaw) to quaternion
    cy = cos(euler(3) * 0.5);
    sy = sin(euler(3) * 0.5);
    cp = cos(euler(2) * 0.5);
    sp = sin(euler(2) * 0.5);
    cr = cos(euler(1) * 0.5);
    sr = sin(euler(1) * 0.5);

    q = [cr * cp * cy + sr * sp * sy;
         sr * cp * cy - cr * sp * sy;
         cr * sp * cy + sr * cp * sy;
         cr * cp * sy - sr * sp * cy];
end