
// include error code definitions
#include "error_codes.h"
// and macros / solver definitions
#include "solver.h"
// include the source rate evaluation header
#include "dydt.h"
// and the struct types
#include "rkf45_types.h"

// check for required defines
#ifndef neq
#pragma error "Number of equations not defined!"
#endif
#ifndef rwk_lensrc
#pragma error "Length of source-rate evaluation working buffer not defined"
#endif
#ifndef __ValueType
#pragma error "Value type not defined!"
#endif

// \brief Indexing macro for _all_ arrays in RKF45 solver (all arrays are size neq)
#define __getIndex(idx) (__getIndex1D(neq, idx))


// Single-step function
int rkf45 (const __ValueType h, const __ValueType t,
           __global const __ValueType* __restrict__ y,
           __global __ValueType* __restrict__ y_out,
           __global __ValueType* __restrict__ rwk,
           __global __ValueType const * __restrict__ user_data)
{
    #define c20 ( 0.25)
    #define c21 ( 0.25)
    #define c30 ( 0.375)
    #define c31 ( 0.09375)
    #define c32 ( 0.28125)
    #define c40 ( 0.92307692307692)
    #define c41 ( 0.87938097405553)
    #define c42 (-3.2771961766045)
    #define c43 ( 3.3208921256258)
    #define c51 ( 2.0324074074074)
    #define c52 (-8.0)
    #define c53 ( 7.1734892787524)
    #define c54 (-0.20589668615984)
    #define c60 ( 0.5)
    #define c61 (-0.2962962962963)
    #define c62 ( 2.0)
    #define c63 (-1.3816764132554)
    #define c64 ( 0.45297270955166)
    #define c65 (-0.275)
    #define a1 ( 0.11574074074074)
    #define a2 ( 0.0)
    #define a3 ( 0.54892787524366)
    #define a4 ( 0.535722994391612)
    #define a5 (-0.2)
    #define b1 ( 0.11851851851852)
    #define b2 ( 0.0)
    #define b3 ( 0.51898635477583)
    #define b4 ( 0.50613149034201)
    #define b5 (-0.18)
    #define b6 ( 0.036363636363636)

    #define h1 (0)
    #define h2 (0.25)
    #define h3 (0.375)
    #define h4 (0.92307692307692307) // 12 / 13
    #define h5 (1.0)
    #define h6 (0.5)
    /*   const double c20 = 0.25,
                            c21 = 0.25,
                            c30 = 0.375,
                            c31 = 0.09375,
                            c32 = 0.28125,
                            c40 = 0.92307692307692,
                            c41 = 0.87938097405553,
                            c42 =-3.2771961766045,
                            c43 = 3.3208921256258,
                            c51 = 2.0324074074074,
                            c52 =-8.0,
                            c53 = 7.1734892787524,
                            c54 =-0.20589668615984,
                            c60 = 0.5,
                            c61 =-0.2962962962963,
                            c62 = 2.0,
                            c63 =-1.3816764132554,
                            c64 = 0.45297270955166,
                            c65 =-0.275,
                            a1 = 0.11574074074074,
                            a2 = 0.0,
                            a3 = 0.54892787524366,
                            a4 = 0.5353313840156,
                            a5 =-0.2,
                            b1 = 0.11851851851852,
                            b2 = 0.0,
                            b3 = 0.51898635477583,
                            b4 = 0.50613149034201,
                            b5 =-0.18,
                            b6 = 0.036363636363636;*/

    // local dependent variables (5 total)
    __global __ValueType* f1   = rwk ;
    __global __ValueType* f2   = rwk + __getOffset1D(neq);
    __global __ValueType* f3   = rwk + __getOffset1D(2*neq);
    __global __ValueType* f4   = rwk + __getOffset1D(3*neq);
    __global __ValueType* f5   = rwk + __getOffset1D(4*neq);
    __global __ValueType* f6   = rwk + __getOffset1D(5*neq);
    __global __ValueType* ytmp = rwk + __getOffset1D(6*neq);
    // the portion of the rwk vector that's allocated for the source rate evaluation
    // y_out is at 7 * neq, hence we go to 8 for the total offset
    __global __ValueType* rwk_dydt = rwk + __getOffset1D(7*neq);

    // 1)
    dydt(t, user_data, y, f1, rwk_dydt);

    for (int k = 0; k < neq; k++)
    {
        //f1[k] = h * ydot[k];
        f1[__getIndex(k)] *= h;
        ytmp[__getIndex(k)] = y[__getIndex(k)] + c21 * f1[__getIndex(k)];
    }

    // 2)
    dydt(t + h2 * h, user_data, ytmp, f2, rwk_dydt);

    for (int k = 0; k < neq; k++)
    {
        //f2[k] = h * ydot[k];
        f2[__getIndex(k)] *= h;
        ytmp[__getIndex(k)] = y[__getIndex(k)] + c31 * f1[__getIndex(k)] + c32 * f2[__getIndex(k)];
    }

    // 3)
    dydt(t + h3 * h, user_data, ytmp, f3, rwk_dydt);

    for (int k = 0; k < neq; k++) {
        //f3[k] = h * ydot[k];
        f3[__getIndex(k)] *= h;
        ytmp[__getIndex(k)] = y[__getIndex(k)] + c41 * f1[__getIndex(k)] + c42 * f2[__getIndex(k)] + c43 * f3[__getIndex(k)];
    }

    // 4)
    dydt(t + h4 * h, user_data, ytmp, f4, rwk_dydt);

    for (int k = 0; k < neq; k++) {
        //f4[k] = h * ydot[k];
        f4[__getIndex(k)] *= h;
        ytmp[__getIndex(k)] = y[__getIndex(k)] + c51 * f1[__getIndex(k)] + c52 * f2[__getIndex(k)] + c53 * f3[__getIndex(k)] + c54 * f4[__getIndex(k)];
    }

    // 5)
    dydt(t + h5 * h, user_data, ytmp, f5, rwk_dydt);

    for (int k = 0; k < neq; k++) {
        //f5[k] = h * ydot[k];
        f5[__getIndex(k)] *= h;
        ytmp[__getIndex(k)] = y[__getIndex(k)] + c61*f1[__getIndex(k)] + c62*f2[__getIndex(k)] + c63*f3[__getIndex(k)] + c64*f4[__getIndex(k)] + c65*f5[__getIndex(k)];
    }

    // 6)
    dydt(t + h6 * h, user_data, ytmp, f6, rwk_dydt);

    for (int k = 0; k < neq; k++)
    {
        //const T f6 = h * ydot[k];
        f6[__getIndex(k)] *= h;

        // 5th-order RK value.
        const __ValueType r5 = b1*f1[__getIndex(k)] + b3*f3[__getIndex(k)] + b4*f4[__getIndex(k)] + b5*f5[__getIndex(k)] + b6*f6[__getIndex(k)];

        // 4th-order RK residual.
        const __ValueType r4 = a1*f1[__getIndex(k)] + a3*f3[__getIndex(k)] + a4*f4[__getIndex(k)] + a5*f5[__getIndex(k)];

        // Trucation error: difference between 4th and 5th-order RK values.
        rwk[__getIndex(k)] = fabs(r5 - r4);

        // Update solution.
        y_out[__getIndex(k)] = y[__getIndex(k)] + r5; // Local extrapolation
    }

    return SUCCESS;

    #undef c20
    #undef c21
    #undef c30
    #undef c31
    #undef c32
    #undef c40
    #undef c41
    #undef c42
    #undef c43
    #undef c51
    #undef c52
    #undef c53
    #undef c54
    #undef c60
    #undef c61
    #undef c62
    #undef c63
    #undef c64
    #undef c65
    #undef a1
    #undef a2
    #undef a3
    #undef a4
    #undef a5
    #undef b1
    #undef b2
    #undef b3
    #undef b4
    #undef b5
    #undef b6
}

__ValueType rk_wnorm (__global const rk_t *rk, __global const __ValueType *x, __global const __ValueType *y)
{
    __ValueType sum = 0;
    for (int k = 0; k < neq; k++)
    {
        __ValueType ewt = (rk->s_rtol * fabs(y[__getIndex(k)])) + rk->s_atol;
        __ValueType prod = x[__getIndex(k)] / ewt;
        sum += (prod*prod);
    }

    return sqrt(sum / (__ValueType)neq);
}

__ValueType rk_hin (__global const rk_t *rk, const __ValueType t, const __ValueType t_end,
                    __ValueType* __restrict__ h0, __global __ValueType* __restrict__ y,
                    __global __ValueType * __restrict__ rwk,
                    __global __ValueType const * __restrict__ user_data)
{
    #define t_round ((t_end - t) * DBL_EPSILON)
    #define h_min (t_round * 100)
    #define h_max ((t_end - t) / rk->min_iters)

    if (__any((t_end - t) < 2 * t_round))
    {
        // requested time-step is smaller than roundoff
        return TDIST_TOO_SMALL;
    }

    __global __ValueType * __restrict__ ydot  = rwk;
    __global __ValueType * __restrict__ y1    = ydot + __getOffset1D(neq);
    __global __ValueType * __restrict__ ydot1 = y1 + __getOffset1D(neq);
     // the portion of the rwk vector that's allocated for the source rate evaluation
    // y_out is at 7 * neq, hence we go to 8 for the total offset
    __global __ValueType* rwk_dydt = rwk + __getOffset1D(7*neq);

    __ValueType hlb = h_min;
    __ValueType hub = h_max;
    //double hlb = h_min;
    //double hub = h_max;

    // Already done ...
    __MaskType done = isgreaterequal(*h0, h_min);
    __ValueType hg = sqrt(hlb*hub);

    if (__all(hub < hlb))
    {
        *h0 = __select(hg, *h0, done);
        return SUCCESS;
    }

    // Start iteration to find solution to ... {WRMS norm of (h0^2 y'' / 2)} = 1

    __MaskType hnew_is_ok = 0;
    __ValueType hnew = hg;
    const int miters = 10;
    int iter = 0;
    int ierr = SUCCESS;

    // compute ydot at t=t0
    dydt(t, user_data, y, ydot, rwk_dydt);

    // maximum of 2 iterations
    #define MAX_HINIT_ITERS (1)
    for(; iter < MAX_HINIT_ITERS; ++iter)
    {
        // Estimate y'' with finite-difference ...
        //double t1 = hg;

        #ifdef __INTEL_COMPILER
        #pragma ivdep
        #endif
        for (int k = 0; k < neq; k++)
        {
            y1[__getIndex(k)] = y[__getIndex(k)] + hg * ydot[__getIndex(k)];
        }

        // compute y' at t1
        dydt(t, user_data, y1, ydot1, rwk_dydt);

        // Compute WRMS norm of y''
        #ifdef __INTEL_COMPILER
        #pragma ivdep
        #endif
        for (int k = 0; k < neq; k++)
            y1[__getIndex(k)] = (ydot1[__getIndex(k)] - ydot[__getIndex(k)]) / hg;

        __ValueType yddnrm = rk_wnorm(rk, y1, y);

        // should we accept this?
        hnew = __select(hnew, hg, hnew_is_ok | (iter == miters));
        if (__all(hnew_is_ok) || (iter == miters))
            break;

        // Get the new value of h ...
        __MaskType test = isgreater(yddnrm*hub*hub, 2.0);
        hnew = __select(sqrt(hg * hub), sqrt(2.0 / yddnrm), test);
        // test the stopping conditions.
        __ValueType hrat = hnew / hg;

        // Accept this value ... the bias factor should bring it within range.
        hnew_is_ok = isgreater(hrat, 0.5) & isless(hrat, 2.0);

        // If y'' is still bad after a few iterations, just accept h and give up.
        if (iter > 1)
        {
            hnew_is_ok = isgreater(hrat, 2.0);
            hnew = __select (hnew, hg, hnew_is_ok);
        }

        hg = hnew;
    }

    // bound and bias estimate
    *h0 = hnew * 0.5;
    *h0 = fmax(*h0, hlb);
    *h0 = fmin(*h0, hub);

    #undef t_round
    #undef h_min
    #undef h_max

    return ierr;
}

//! \brief struct containing information on steps / iterations
typedef struct
{
    int niters;
    __MaskType nsteps;
}
rk_counters_t_vec;


__IntType rk_solve (__global const rk_t * __restrict__ rk,
                    __private __ValueType const t_start,
                    __private __ValueType const t_end,
                    __private __ValueType hcur,
                    __private rk_counters_t_vec * __restrict__ counters,
                    __global __ValueType* __restrict__ y,
                    __global __ValueType* __restrict__ rwk,
                    __global __ValueType const * __restrict__ user_data)
{

    __ValueType t = t_start;
    #define t_round ((t_end - t_start) * DBL_EPSILON)
    #define h_min (t_round * 100)
    #define h_max ((t_end - t_start) / rk->min_iters)
    #define iter (counters->niters)
    #define nst (counters->nsteps)

    __IntType ierr = SUCCESS;
    // Estimate the initial step size ...
    {
        __MaskType test = isless(hcur, h_min);
        if (__any(test))
        {
            ierr = rk_hin(rk, t, t_end, &hcur, y, rwk + __getOffset1D(neq), user_data);
        }
    }

    nst = 0;
    iter = 0;

    __MaskType done = isless(fabs(t - t_end), fabs(t_round));
    __global __ValueType *ytmp = rwk;

    while (__any(__not(done)))
    {

        // Take a trial step over h_cur ...
        rkf45(hcur, t, y, ytmp, rwk + __getOffset1D(neq), user_data);

        __ValueType herr = fmax(1e-20, rk_wnorm(rk, rwk + __getOffset1D(neq), y));

        // Is there error acceptable?
        __MaskType accept = islessequal(herr, 1.0);
        accept |= islessequal(hcur, h_min);
        accept &= __not(done);

        // update solution ...
        if (__any(accept))
        {
            t   = __select (t,   t + hcur  , accept);
            nst = __select (nst, nst + 1, accept);

            for (int k = 0; k < neq; k++)
                y[__getIndex(k)] = __select(y[__getIndex(k)], ytmp[__getIndex(k)], accept);

            done = isless( fabs(t - t_end), fabs(t_round));
        }

        __ValueType fact = sqrt( sqrt(1.0 / herr) ) * (0.840896415);

        // Restrict the rate of change in dt
        fact = fmax(fact, 1.0 / rk->adaption_limit);
        fact = fmin(fact,       rk->adaption_limit);

        // Apply grow/shrink factor for next step.
        hcur = __select(hcur * fact, hcur, done);

        // Limit based on the upper/lower bounds
        hcur = fmin(hcur, h_max);
        hcur = fmax(hcur, h_min);

        // Stretch the final step if we're really close and we didn't just fail ...
        hcur = __select(hcur, t_end - t, accept & isless(fabs((t + hcur) - t_end), h_min));

        // Don't overshoot the final time ...
        hcur = __select(hcur, t_end - t, __not(done) & isgreater((t + hcur), t_end));

        ++iter;
        if (rk->max_iters && iter > rk->max_iters) {
            ierr = TOO_MUCH_WORK;
            //printf("(iter > max_iters)\n");
            break;
        }
    }

    return ierr;

    #undef t_round
    #undef h_min
    #undef h_max
    #undef iter
    #undef nst
}
