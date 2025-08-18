function [xest, Jopt, P, itermflag] = gauss_newton_solver(H_func, h_x, z_data, t_data, R, x_init_guess, max_num_iterations, idispflag)
x_guess = x_init_guess;
Ra = chol(R);
Ra_inv = inv(Ra)';

J = J_NLLS(x_guess);
[d_x, P, d_Jpred] = compute_step;
delJsizetest = abs(d_Jpred) < 1.e-13*(1 + J);
delxsizetest = norm(d_x) < 1.e-09*(1 + norm(x_guess));
if delJsizetest && delxsizetest
  xest = x_guess;
  Jopt = J;
  return
end

itermflag = 0;
niteration = 0;
test_done = false;
while ~test_done

    alpha = 1;
    x_new = x_guess + alpha*d_x;
    J_new = J_NLLS(x_new);
    alpha_iter = 0;
    while J_new > J
        alpha = 0.5*alpha;
        x_new = x_guess + alpha*d_x;
        J_new = J_NLLS(x_new);
        alpha_iter = alpha_iter + 1;
        if (alpha_iter > 50)
            itermflag = 1;
            test_done = true;
        end
    end
    x_guess = x_new;
    Jold = J;
    delJold = J_new - J;
    delJpredold = d_Jpred;

    [d_x, P, d_Jpred] = compute_step;
    J = J_new;

    delJsizetest = abs(d_Jpred) < 1.e-13*(1 + J);
    delxsizetest = norm(d_x) < 1.e-09*(1 + norm(x_guess));
    alpha_test = alpha == 1;
    delJratiotest = abs((delJold/delJpredold) - 1) < 0.01;

    if (delJsizetest && delxsizetest) || (alpha_test && delJratiotest && delxsizetest)
        test_done = true;
    end
    niteration = niteration + 1;                
    if test_done == false && niteration >= max_num_iterations
        itermflag = 2;
        test_done = true;
    end
    if idispflag == 1
        disp([' At iteration ',int2str(niteration),' alpha = ',...
              num2str(alpha),', Jnew = ',num2str(J),', Jold = ',...
              num2str(Jold),', and norm(delxnew) = ',...
              num2str(norm(d_x)),'.'])
    end

    
end
xest = x_guess;
Jopt = J;

    function J_cost = J_NLLS(x)
        za = Ra_inv*z_data;
        ha = Ra_inv*h_x(x, t_data);
        J_cost = 0.5*(za - ha)'*(za - ha);
    end

    function [del_x, P_est, delJpred] = compute_step
        Ha = Ra_inv*H_func(x_guess, t_data);
        za = Ra_inv*z_data;
        ha = Ra_inv*h_x(x_guess, t_data);
        [Qb, Rb] = qr(Ha,0);
        Rb_inv = inv(Rb);
        del_x = Rb_inv*Qb'*(za-ha);
        P_est = Rb_inv*Rb_inv';
        dJdx = (Ha')*(za-ha);
        dJdalpha = (dJdx')*del_x;
        dum = Rb*del_x;
        d2Jdalpha2 = (dum')*dum;
        delJpred = dJdalpha + .5*d2Jdalpha2;
    end
end