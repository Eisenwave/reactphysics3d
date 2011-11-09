/********************************************************************************
* ReactPhysics3D physics library, http://code.google.com/p/reactphysics3d/      *
* Copyright (c) 2010 Daniel Chappuis                                            *
*********************************************************************************
*                                                                               *
* Permission is hereby granted, free of charge, to any person obtaining a copy  *
* of this software and associated documentation files (the "Software"), to deal *
* in the Software without restriction, including without limitation the rights  *
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell     *
* copies of the Software, and to permit persons to whom the Software is         *
* furnished to do so, subject to the following conditions:                      *
*                                                                               *
* The above copyright notice and this permission notice shall be included in    *
* all copies or substantial portions of the Software.                           *
*                                                                               *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR    *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,      *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE   *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER        *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN     *
* THE SOFTWARE.                                                                 *
********************************************************************************/

#ifndef CONSTRAINT_SOLVER_H
#define CONSTRAINT_SOLVER_H

// Libraries
#include "../constants.h"
#include "../constraint/Constraint.h"
#include "../mathematics/lcp/LCPSolver.h"
#include "ContactCache.h"
#include "PhysicsWorld.h"
#include <map>
#include <set>
#include <sys/time.h> // TODO : Remove this

// ReactPhysics3D namespace
namespace reactphysics3d {
    
    
/*  -------------------------------------------------------------------
    Class ConstrainSolver :
        This class represents the constraint solver. The constraint solver
        is based on the theory from the paper "Iterative Dynamics with
        Temporal Coherence" from Erin Catto. We keep the same notations as
        in the paper. The idea is to construct a LCP problem and then solve
        it using a Projected Gauss Seidel (PGS) solver.
    -------------------------------------------------------------------
*/
class ConstraintSolver {
    private:
        PhysicsWorld* physicsWorld;                     // Reference to the physics world
        std::vector<Constraint*> activeConstraints;     // Current active constraints in the physics world
        uint nbIterationsLCP;                           // Number of iterations of the LCP solver
        uint nbConstraints;                             // Total number of constraints (with the auxiliary constraints)
        uint nbBodies;                                  // Current number of bodies in the physics world
        double penetrationFactor;                       // Penetration factor "beta" for penetration correction
        std::set<Body*> constraintBodies;               // Bodies that are implied in some constraint
        std::map<Body*, uint> bodyNumberMapping;        // Map a body pointer with its index number
        Body* bodyMapping[NB_MAX_CONSTRAINTS][2];       // 2-dimensional array that contains the mapping of body reference
                                                        // in the J_sp and B_sp matrices. For instance the cell bodyMapping[i][j] contains
                                                        // the pointer to the body that correspond to the 1x6 J_ij matrix in the
                                                        // J_sp matrix. An integer body index refers to its index in the "bodies" std::vector
        double J_sp[NB_MAX_CONSTRAINTS][2*6];           // 2-dimensional array that correspond to the sparse representation of the jacobian matrix of all constraints
                                                        // This array contains for each constraint two 1x6 Jacobian matrices (one for each body of the constraint)
                                                        // a 1x6 matrix
        double B_sp[2][6*NB_MAX_CONSTRAINTS];           // 2-dimensional array that correspond to a useful matrix in sparse representation
                                                        // This array contains for each constraint two 6x1 matrices (one for each body of the constraint)
                                                        // a 6x1 matrix
        double b[NB_MAX_CONSTRAINTS];                   // Vector "b" of the LCP problem
        double d[NB_MAX_CONSTRAINTS];                   // Vector "d"
        double a[6*NB_MAX_BODIES];                      // Vector "a"
        double lambda[NB_MAX_CONSTRAINTS];              // Lambda vector of the LCP problem
        double lambdaInit[NB_MAX_CONSTRAINTS];          // Lambda init vector for the LCP solver
        double errorValues[NB_MAX_CONSTRAINTS];         // Error vector of all constraints
        double lowerBounds[NB_MAX_CONSTRAINTS];         // Vector that contains the low limits for the variables of the LCP problem
        double upperBounds[NB_MAX_CONSTRAINTS];         // Vector that contains the high limits for the variables of the LCP problem
        Matrix3x3 Minv_sp_inertia[NB_MAX_BODIES];       // 3x3 world inertia tensor matrix I for each body (from the Minv_sp matrix)
        double Minv_sp_mass_diag[NB_MAX_BODIES];        // Array that contains for each body the inverse of its mass
                                                        // This is an array of size nbBodies that contains in each cell a 6x6 matrix
        double V1[6*NB_MAX_BODIES];                     // Array that contains for each body the 6x1 vector that contains linear and angular velocities
                                                        // Each cell contains a 6x1 vector with linear and angular velocities
        double Vconstraint[6*NB_MAX_BODIES];            // Same kind of vector as V1 but contains the final constraint velocities
        double Fext[6*NB_MAX_BODIES];                   // Array that contains for each body the 6x1 vector that contains external forces and torques
                                                        // Each cell contains a 6x1 vector with external force and torque.
        void initialize();                              // Initialize the constraint solver before each solving
        void fillInMatrices();                          // Fill in all the matrices needed to solve the LCP problem
        void computeVectorB(double dt);                 // Compute the vector b
        void computeMatrixB_sp();                       // Compute the matrix B_sp
        void computeVectorVconstraint(double dt);       // Compute the vector V2
        void cacheLambda();                             // Cache the lambda values in order to reuse them in the next step to initialize the lambda vector
        void computeVectorA();                          // Compute the vector a used in the solve() method
        void solveLCP();                                // Solve a LCP problem using Projected-Gauss-Seidel algorithm
        
    public:
        ConstraintSolver(PhysicsWorld* world);                      // Constructor
        virtual ~ConstraintSolver();                                // Destructor
        void solve(double dt);                                      // Solve the current LCP problem
        bool isConstrainedBody(Body* body) const;                   // Return true if the body is in at least one constraint
        Vector3 getConstrainedLinearVelocityOfBody(Body* body);     // Return the constrained linear velocity of a body after solving the LCP problem
        Vector3 getConstrainedAngularVelocityOfBody(Body* body);    // Return the constrained angular velocity of a body after solving the LCP problem
        void cleanup();                                             // Cleanup of the constraint solver
        void setPenetrationFactor(double penetrationFactor);        // Set the penetration factor 
        void setNbLCPIterations(uint nbIterations);                 // Set the number of iterations of the LCP solver
};

// Return true if the body is in at least one constraint
inline bool ConstraintSolver::isConstrainedBody(Body* body) const {
    if(constraintBodies.find(body) != constraintBodies.end()) {
        return true;
    }
    return false;
}

// Return the constrained linear velocity of a body after solving the LCP problem
inline Vector3 ConstraintSolver::getConstrainedLinearVelocityOfBody(Body* body) {
    assert(isConstrainedBody(body));
    uint indexBodyArray = 6 * bodyNumberMapping[body];
    return Vector3(Vconstraint[indexBodyArray], Vconstraint[indexBodyArray + 1], Vconstraint[indexBodyArray + 2]);
}


// Return the constrained angular velocity of a body after solving the LCP problem
inline Vector3 ConstraintSolver::getConstrainedAngularVelocityOfBody(Body* body) {
    assert(isConstrainedBody(body));
    uint indexBodyArray = 6 * bodyNumberMapping[body];
    return Vector3(Vconstraint[indexBodyArray + 3], Vconstraint[indexBodyArray + 4], Vconstraint[indexBodyArray + 5]);
}

// Cleanup of the constraint solver
inline void ConstraintSolver::cleanup() {
    bodyNumberMapping.clear();
    constraintBodies.clear();
    activeConstraints.clear();
}

// Set the penetration factor 
inline void ConstraintSolver::setPenetrationFactor(double factor) {
    penetrationFactor = factor;
}   

// Set the number of iterations of the LCP solver
inline void ConstraintSolver::setNbLCPIterations(uint nbIterations) {
    nbIterationsLCP = nbIterations;
}                 

// Solve the current LCP problem
inline void ConstraintSolver::solve(double dt) {

    // TODO : Remove the following timing code
    timeval timeValueStart;
	timeval timeValueEnd;
	std::cout << "------ START (Constraint Solver) -----" << std::endl;
	gettimeofday(&timeValueStart, NULL);
    
    // Allocate memory for the matrices
    initialize();

    // Fill-in all the matrices needed to solve the LCP problem
    fillInMatrices();

    // Compute the vector b
    computeVectorB(dt);

    // Compute the matrix B
    computeMatrixB_sp();

    // Solve the LCP problem (computation of lambda)
    solveLCP();

    // Cache the lambda values in order to use them in the next step
    cacheLambda();
    
    // Compute the vector Vconstraint
    computeVectorVconstraint(dt);
    
    // TODO : Remove the following timing code
    std::cout << "NB constraints : " << nbConstraints << std::endl;
    gettimeofday(&timeValueEnd, NULL);
	long double startTime = timeValueStart.tv_sec * 1000000.0 + (timeValueStart.tv_usec);
	long double endTime = timeValueEnd.tv_sec * 1000000.0 + (timeValueEnd.tv_usec);
	std::cout << "------ END (Constraint Solver) => (" << "time = " << endTime - startTime << " micro sec)-----" << std::endl;
}

} // End of ReactPhysics3D namespace

#endif