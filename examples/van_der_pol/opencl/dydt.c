/**
 * \file
 * \brief An implementation of the van der Pol right hand side (y' = f(y)) function.
 *
 * Implements the evaluation of the derivative of the state vector with respect to time, \f$\dot{\vec{y}}\f$
 * for OpenCL
 *
 */

// include indexing macros for OpenCL to make life easy
#include "dydt.h"

/**
 * The `y` and `dy` vectors supplied here are local versions of the global state vectors.
 * They have been transformed from the global Column major (Fortan) ordering to a local 1-D vector.
 * Hence the vector accesses can be done in a simple manner below, i.e. y[0] -> \f$y_1\f$, y[1] -> \f$y_2\f$, etc.
 * @see solver_generic.c
 */
void dydt (const __ValueType* t, const __ValueType* mu, const __ValueType * __restrict__ y, __ValueType * __restrict__ dy) {

  // y1' = y2
  dy[__getIndex(0)] = y[__getIndex(1)];
  // y2' = mu(1 - y1^2)y2 - y1
  dy[__getIndex(1)] =  * (1 - y[__getIndex(0)] * y[__getIndex(0)]) * y[__getIndex(1)] - y[__getIndex(0)];

} // end dydt
