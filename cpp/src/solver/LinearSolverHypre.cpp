#ifdef USE_HYPRE

////////////////////////////////////////////////////////////////////////////////
#include "LinearSolverHypre.hpp"

#include <HYPRE_krylov.h>

////////////////////////////////////////////////////////////////////////////////

using namespace poly_fem;

////////////////////////////////////////////////////////////////////////////////

LinearSolverHypre::LinearSolverHypre()
{
}


// Set solver parameters
void LinearSolverHypre::setParameters(const json &params) {
// if (params.count("mtype")) {
// 	setType(params["mtype"].get<int>());
// }
}

void LinearSolverHypre::getInfo(json &params) const {
	params["num_iterations"] = num_iterations;
	params["final_res_norm"] = final_res_norm;
}



////////////////////////////////////////////////////////////////////////////////

void LinearSolverHypre::analyzePattern(const SparseMatrixXd &Ain) {
	if(has_matrix_){
		HYPRE_IJMatrixDestroy(A);
		has_matrix_ = false;
	}

	has_matrix_ = true;
	const HYPRE_Int rows = Ain.rows();
	const HYPRE_Int cols = Ain.cols();

	HYPRE_IJMatrixCreate(MPI_COMM_WORLD, 0, rows, 0, cols, &A);
	// HYPRE_IJMatrixSetPrintLevel(A, 2);
	HYPRE_IJMatrixSetObjectType(A, HYPRE_PARCSR);
	HYPRE_IJMatrixInitialize(A);


	// HYPRE_IJMatrixSetValues(A, 1, &nnz, &i, cols, values);

	for (HYPRE_Int k = 0; k < Ain.outerSize(); ++k) {
		for (Eigen::SparseMatrix<double>::InnerIterator it(Ain, k); it; ++it) {
			const HYPRE_Int 	i[1] = {it.row()};
			const HYPRE_Int 	j[1] = {it.col()};
			const HYPRE_Complex v[1] = {it.value()};
			HYPRE_Int n_cols[1] = {1};

			HYPRE_IJMatrixSetValues(A, 1, n_cols, i, j, v);
		}
	}

	HYPRE_IJMatrixAssemble(A);
	HYPRE_IJMatrixGetObject(A, (void**) &parcsr_A);
}


////////////////////////////////////////////////////////////////////////////////

void LinearSolverHypre::solve(const Eigen::Ref<const VectorXd> rhs, Eigen::Ref<VectorXd> result) {
	HYPRE_IJVector b;
	HYPRE_ParVector par_b;
	HYPRE_IJVector x;
	HYPRE_ParVector par_x;

	HYPRE_IJVectorCreate(MPI_COMM_WORLD, 0, rhs.size(), &b);
	HYPRE_IJVectorSetObjectType(b, HYPRE_PARCSR);
	HYPRE_IJVectorInitialize(b);

	HYPRE_IJVectorCreate(MPI_COMM_WORLD, 0, rhs.size(), &x);
	HYPRE_IJVectorSetObjectType(x, HYPRE_PARCSR);
	HYPRE_IJVectorInitialize(x);


	for(HYPRE_Int i = 0; i < rhs.size(); ++i)
	{
		const HYPRE_Int 	index[1] = {i};
		const HYPRE_Complex v[1] = {HYPRE_Complex(rhs(i))};
		const HYPRE_Complex z[1] = {HYPRE_Complex(0)};

		HYPRE_IJVectorSetValues(b, 1, index, v);
		HYPRE_IJVectorSetValues(x, 1, index, z);
	}

	HYPRE_IJVectorAssemble(b);
	HYPRE_IJVectorGetObject(b, (void **) &par_b);

	HYPRE_IJVectorAssemble(x);
	HYPRE_IJVectorGetObject(x, (void **) &par_x);


	/* PCG with AMG preconditioner */

	/* Create solver */
	HYPRE_Solver solver, precond;
	HYPRE_ParCSRPCGCreate(MPI_COMM_WORLD, &solver);

	/* Set some parameters (See Reference Manual for more parameters) */
	HYPRE_PCGSetMaxIter(solver, max_iter_); /* max iterations */
	HYPRE_PCGSetTol(solver, conv_tol_); /* conv. tolerance */
	HYPRE_PCGSetTwoNorm(solver, 1); /* use the two norm as the stopping criteria */
	//HYPRE_PCGSetPrintLevel(solver, 2); /* print solve info */
	HYPRE_PCGSetLogging(solver, 1); /* needed to get run info later */

	/* Now set up the AMG preconditioner and specify any parameters */
	HYPRE_BoomerAMGCreate(&precond);
	//HYPRE_BoomerAMGSetPrintLevel(precond, 2); /* print amg solution info */
	HYPRE_BoomerAMGSetCoarsenType(precond, 6);
	HYPRE_BoomerAMGSetOldDefault(precond);
	HYPRE_BoomerAMGSetRelaxType(precond, 6); /* Sym G.S./Jacobi hybrid */
	HYPRE_BoomerAMGSetNumSweeps(precond, 1);
	HYPRE_BoomerAMGSetTol(precond, 0.0); /* conv. tolerance zero */
	HYPRE_BoomerAMGSetMaxIter(precond, pre_max_iter_); /* do only one iteration! */

	/* Set the PCG preconditioner */
	HYPRE_PCGSetPrecond(solver, (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSolve, (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSetup, precond);

	/* Now setup and solve! */
	HYPRE_ParCSRPCGSetup(solver, parcsr_A, par_b, par_x);
	HYPRE_ParCSRPCGSolve(solver, parcsr_A, par_b, par_x);

	/* Run info - needed logging turned on */
	HYPRE_PCGGetNumIterations(solver, &num_iterations);
	HYPRE_PCGGetFinalRelativeResidualNorm(solver, &final_res_norm);

	// printf("\n");
	// printf("Iterations = %lld\n", num_iterations);
	// printf("Final Relative Residual Norm = %g\n", final_res_norm);
	// printf("\n");

	/* Destroy solver and preconditioner */
	HYPRE_BoomerAMGDestroy(precond);
	HYPRE_ParCSRPCGDestroy(solver);


	assert(result.size() == rhs.size());
	for(HYPRE_Int i = 0; i < rhs.size(); ++i){
		const HYPRE_Int 	index[1] = {i};
		HYPRE_Complex 		v[1];
		HYPRE_IJVectorGetValues(x, 1, index, v);

		result(i) = v[0];
	}

	HYPRE_IJVectorDestroy(b);
	HYPRE_IJVectorDestroy(x);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

LinearSolverHypre::~LinearSolverHypre() {
	if(has_matrix_){
		HYPRE_IJMatrixDestroy(A);
		has_matrix_ = false;
	}
}

#endif
