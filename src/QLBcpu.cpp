/**
 *	Quantum Lattice Boltzmann 
 *	(c) 2015 Fabian Thüring, ETH Zurich
 *
 *	This file contains the CPU implementations of the QLB scheme
 *	
 *	Based on the implementation of M. J. Miller (ETH Zurich)
 */

#include "QLB.hpp"

// === Potential ===

QLB::float_t QLB::V_harmonic(int i, int j) const 
{
	const float_t w0 = 1.0 / (2.0*mass_*delta0_*delta0_);
	const float_t x  = dx_*(i-0.5*(L_-1));
	const float_t y  = dx_*(j-0.5*(L_-1));

	return -0.5*mass_*w0*w0*(x*x + y*y);
}

QLB::float_t QLB::V_free(int i, int j) const
{
	return float_t(0);
}

// === Simulation ===

/**
 *	The RHS of the eqauation is (rotated collision matrices):
 *
 *	X * ( i*g*I + i*wc*Beta ) * Xinv
 *
 *	   =      i*g           0           0       wc*i
 *	            0         i*g      2*wc*i       wc*i
 *      -0.5*wc*i    0.5*wc*i         i*g          0
 *           wc*i           0           0        i*g
 */

void QLB::Qhat_X(int i, int j, QLB::cmat_t& Q) const
{
	float_t V_ij;
	if(V_indx_ == 0)
		V_ij = V_harmonic(i,j);
	else
		V_ij = V_free(i,j); 

	// Precompute frequently used values
	const float_t m = 0.5 * mass_* dt_;
	const float_t g = 0.5 * V_ij * dt_;
	const float_t omega = m*m - g*g;

	const complex_t a = (one - float_t(0.25)*omega) / 
	                    (one + float_t(0.25)*omega - img*g);
	const complex_t b = m / (one + float_t(0.25)*omega - img*g);

	// Qhat = X^(-1) * Q * X
	Q(0,0) =  a;
	Q(0,1) =  0;
	Q(0,2) =  0;
	Q(0,3) =  b*img;

	Q(1,0) =  0;
	Q(1,1) =  a;
	Q(1,2) =  float_t(2.0)*b*img;
	Q(1,3) =  b*img;

	Q(2,0) = -float_t(0.5)*b*img;
	Q(2,1) =  float_t(0.5)*b*img;
	Q(2,2) =  a;
	Q(2,3) =  0;

	Q(3,0) =  b*img;
	Q(3,1) =  0;
	Q(3,2) =  0;
	Q(3,3) =  a;
}


void QLB::Qhat_Y(int i, int j, QLB::cmat_t& Q) const
{
	// Precompute frequently used values
	float_t V_ij;
	if(V_indx_ == 0)
		V_ij = V_harmonic(i,j);
	else
		V_ij = V_free(i,j); 

	const float_t m = 0.5 * mass_* dt_;
	const float_t g = 0.5 * V_ij * dt_;
	const float_t omega = m*m - g*g;

	const complex_t a = (one - float_t(0.25)*omega) / 
	                    (one + float_t(0.25)*omega - img*g);
	const complex_t b = m / (one + float_t(0.25)*omega - img*g);

	// Qhat = Y^(-1) * Q * Y
	Q(0,0) =  a;
	Q(0,1) =  0;
	Q(0,2) =  0;
	Q(0,3) =  b*img;

	Q(1,0) =  0;
	Q(1,1) =  a;
	Q(1,2) =  float_t(2.0)*b*img;
	Q(1,3) =  b*img;

	Q(2,0) = -float_t(0.5)*b*img;
	Q(2,1) =  float_t(0.5)*b*img;
	Q(2,2) =  a;
	Q(2,3) =  0;

	Q(3,0) =  b*img;
	Q(3,1) =  0;
	Q(3,2) =  0;
	Q(3,3) =  a;
}

void QLB::evolution_CPU()
{
	const int L = L_;

	// thread private
	cmat_t Q(4);
	int ia, ja;
	int ik, jk;

	// Rotate with X
	for(int i = 0; i < L; ++i)
	{
		for(int j=0; j < L; ++j)
		{

			for(int mk = 0; mk < 4; ++mk)
				spinorrot_(i,j,mk) = 0;

			for(int mk = 0; mk < 4; ++mk)
				for(int nk = 0; nk < 4; ++nk)
					spinorrot_(i,j,mk) += X(mk,nk) * spinor_(i,j,nk);
		}
	}

	for(int i = 0; i < L; ++i)
	{
		for(int j = 0; j < L; ++j)
		{
			spinoraux_(i,j,0) = spinorrot_(i,j,0);
			spinoraux_(i,j,1) = spinorrot_(i,j,1);
			spinoraux_(i,j,2) = spinorrot_(i,j,2);
			spinoraux_(i,j,3) = spinorrot_(i,j,3);
		}
	}

	// collide & stream with Q 
	for(int i = 0; i < L; ++i)
	{
		for(int j = 0; j < L; ++j)
		{
			ia = (i + 1) % L;
			ik = (i - 1 + L) % L;

			spinorrot_(ia,j,0) = 0;
			spinorrot_(ia,j,1) = 0;
			spinorrot_(ik,j,2) = 0;
			spinorrot_(ik,j,3) = 0;

			Qhat_X(i, j, Q);
	
			for(int nk = 0; nk < 4; ++nk)
			{
				spinorrot_(ia,j,0) += Q(0,nk) * spinoraux_(i,j,nk);
				spinorrot_(ia,j,1) += Q(1,nk) * spinoraux_(i,j,nk);
				spinorrot_(ik,j,2) += Q(2,nk) * spinoraux_(i,j,nk);
				spinorrot_(ik,j,3) += Q(3,nk) * spinoraux_(i,j,nk);
			}

		}
	}

	// Rotate back with Xinv
	for(int i = 0; i < L; ++i)
	{
		for(int j = 0; j < L; ++j)
		{

			for(int mk = 0; mk < 4; ++mk)
				spinor_(i,j,mk) = 0;

			for(int mk = 0; mk < 4; ++mk)
				for(int nk = 0; nk < 4; ++nk)
					spinor_(i,j,mk) += Xinv(mk,nk)*spinorrot_(i,j,nk);
		}
	}

	// Rotate with Y
	for(int i = 0; i < L; ++i)
	{
		for(int j = 0; j < L; ++j)
		{
			for(int mk = 0; mk < 4; ++mk)
				spinorrot_(i,j,mk) = 0;

			for(int mk=0; mk < 4; ++mk)
				for(int nk=0; nk < 4; ++nk)
					spinorrot_(i,j,mk) += Y(mk,nk) * spinor_(i,j,nk);
		}
	}
	
	for(int i = 0; i < L; ++i)
	{
		for(int j = 0; j < L; ++j)
		{
			spinoraux_(i,j,0) = spinorrot_(i,j,0);
			spinoraux_(i,j,1) = spinorrot_(i,j,1);
			spinoraux_(i,j,2) = spinorrot_(i,j,2);
			spinoraux_(i,j,3) = spinorrot_(i,j,3);
		}
	}
		
	// collide & stream with Q
	for(int i = 0; i < L; ++i)
	{
		for(int j = 0; j < L; ++j)
		{

			ja = (j + 1) % L;
			jk = (j - 1 + L) % L;

			spinorrot_(i,ja,0) = 0;
			spinorrot_(i,ja,1) = 0;
			spinorrot_(i,jk,2) = 0;
			spinorrot_(i,jk,3) = 0;

			Qhat_Y(i, j, Q);

			for(int nk = 0; nk < 4; ++nk)
			{
				spinorrot_(i,ja,0) += Q(0,nk) * spinoraux_(i,j,nk);
				spinorrot_(i,ja,1) += Q(1,nk) * spinoraux_(i,j,nk);
				spinorrot_(i,jk,2) += Q(2,nk) * spinoraux_(i,j,nk);
				spinorrot_(i,jk,3) += Q(3,nk) * spinoraux_(i,j,nk);
			}
		}
	}
	
	// Rotate back with Yinv
	for(int i = 0; i < L; ++i)
	{
		for(int j = 0; j < L; ++j)
		{
			for(int mk = 0; mk < 4; ++mk)
				spinor_(i,j,mk) = 0;

			for(int mk = 0; mk < 4; ++mk)
				for(int nk = 0; nk < 4; ++nk)
					spinor_(i,j,mk) += Yinv(mk,nk)*spinorrot_(i,j,nk);
		}
	}

	calculate_macroscopic_vars();

	// Wirte to STDOUT (if requested)
	if(verbose_)
	{
		std::cout << std::setw(15) << t_*dt_;
		std::cout << std::setw(15) << deltax_;
		std::cout << std::setw(15) << deltay_;
		
		if(V_indx_ == 1) // no potential
		{
			float_t deltax_t = std::sqrt( delta0_*delta0_ + t_*dt_*t_*dt_ /
							              (4.0*mass_*mass_*delta0_*delta0_) );
			std::cout << std::setw(15) << deltax_t;
		}
		std::cout << std::endl;
	}
	
	// Wirte to spread.dat (if requested)
	if(plot_[1] || plot_[0])
	{	
		if(t_*dt_ == 0) // Delete the old content
			fout.open("spread.dat");
		else
			fout.open("spread.dat", std::ios::app);
			
		fout << std::setw(15) << t_*dt_;
		fout << std::setw(15) << deltax_;
		fout << std::setw(15) << deltay_;
		
		if(V_indx_ == 1) // no potential
		{
			float_t deltax_t = std::sqrt( delta0_*delta0_ + t_*dt_*t_*dt_ /
							              (4.0*mass_*mass_*delta0_*delta0_) );
			fout << std::setw(15) << deltax_t;
		}
		fout << std::endl;	
		fout.close();
	}
}

void QLB::calculate_macroscopic_vars()
{
	float_t deltax_nom = 0.0, deltax_den = 0.0;
	float_t deltay_nom = 0.0, deltay_den = 0.0;

	float_t x = 0.0, y = 0.0;
	const float_t dV = dx_*dx_;
	
	for(unsigned i = 0; i < L_; ++i)
	{
		for(unsigned j = 0; j < L_; ++j)
		{
			x = dx_*(i-0.5*(L_-1));
			y = dx_*(j-0.5*(L_-1));

			// Delta X
			deltax_nom += x*x*std::norm(spinor_(i,j,0))*dV;
			deltax_den += std::norm(spinor_(i,j,0))*dV;

			// Delta Y
			deltay_nom += y*y*std::norm(spinor_(i,j,0))*dV;
			deltay_den += std::norm(spinor_(i,j,0))*dV;

			currentX_(i,j) = 0;
			currentY_(i,j) = 0;

			// Calculate current:
			// currentX = [s1, s2, s3, s4].H * alphaX * [s1, s2, s3, s4]  	
			for(int is = 0; is < 4; ++is)
				for(int js = 0; js < 4; ++js)
				{
					currentX_(i,j) += 
					  std::conj(spinor_(i,j,is))*alphaX(is,js)*spinor_(i,j,js);
					currentY_(i,j) +=
					  std::conj(spinor_(i,j,is))*alphaY(is,js)*spinor_(i,j,js);
				}
		
			// Calculate density
			rho_(i,j) = std::norm(spinor_(i,j,0)) + std::norm(spinor_(i,j,1)) + 
			            std::norm(spinor_(i,j,2)) + std::norm(spinor_(i,j,3));
			
			veloX_(i,j) = currentX_(i,j)/rho_(i,j);
			veloY_(i,j) = currentY_(i,j)/rho_(i,j);
		}
    }

	// Update global variables
	deltax_ = std::sqrt(deltax_nom/deltax_den);
	deltay_ = std::sqrt(deltay_nom/deltay_den);
}