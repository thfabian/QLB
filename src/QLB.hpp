/**
 *	Quantum Lattice Boltzmann 
 *	(c) 2015 Fabian Thüring, ETH Zurich
 *
 *	Dirac Solver with Quantum Lattice Boltzmann scheme.
 *	This file contains the class definition of QLB.
 *
 *	@References
 *	Isotropy of three-dimensional quantum lattice Boltzmann schemes, 
 *	P.J. Dellar, D. Lapitski, S. Palpacelli, S. Succi, 2011
 */

#ifndef QLB_HPP
#define QLB_HPP

#define _USE_MATH_DEFINES

// System includes
#include <complex>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cmath>

// Local includes
#include "error.hpp"
#include "GLerror.hpp"
#include "utility.hpp"
#include "VBO.hpp"
#include "Shader.hpp"
#include "matrix.hpp"

/********************************
 *  Quantum Lattice Boltzmann   *
 ********************************/
class QLB
{
public:

#ifndef QLB_SINGLE_PRECISION 
	typedef double float_t;
#else
	typedef float float_t;
#endif

	// === typedefs ===
	typedef std::complex<float_t> complex_t;
	typedef std::vector<float_t> fvec_t;
	typedef std::vector<int> ivec_t;
	typedef std::vector<bool> bvec_t;
	typedef matND<complex_t> cmat_t;
	typedef matND<float_t> fmat_t;
	typedef matN4D<complex_t> c4mat_t;
	
	enum scene_t  { spinor0 = 0, spinor1 = 1, spinor2 = 2, spinor3 = 3, potential = 4 };
	enum render_t { SOLID = 0x0005, WIRE = 0x0003 };

	// === Constants ===
	static const complex_t img;
	static const complex_t one;
	
	static const cmat_t X;
	static const cmat_t Y;

	static const cmat_t Xinv;
	static const cmat_t Yinv;
	
	static const cmat_t alphaX;
	static const cmat_t alphaY;

	static const cmat_t beta;
	
	// === Constructor & Destructor ===
	
	/** 
	 *	Constructor 
	 *	@param	L 		Grid size i.e grid will be L x L
	 *	@param 	dx		Spatial discretization
	 *	@param	mass	Mass of the particles
	 *	@param 	dt		Temporal discretization
	 *	@param 	V_indx	Index of the potential function V
	 *	                0: harmonic potential
	 *	                1: no potential
	 *	@param 	plot_	Boolean vector to indicate which quantities are written 
	 *  				to a output file when calling 'write_content_to_file'
	 *	                [ 0] :  all
	 *	                [ 1] :  spread
	 *	                [ 2] :  spinor1
	 *	                [ 3] :  spinor2
	 *	                [ 4] :  spinor3
	 *	                [ 5] :  spinor4
	 *	                [ 6] :  density
	 *	                [ 7] :  currentX
	 *	                [ 8] :  currentY
	 *	                [ 9] :  veloX
	 *	                [10] :  veloY
	 *	@param 	verb	Run in verbose mode (optional : default false)
	 *	@file 	QLB.cpp
	 */
	QLB(unsigned L, float_t dx, float_t mass, float_t dt, int V_indx,
	    const bvec_t& plot, bool verb = false);
	
	/** 
	 *	Destructor
	 *	@file 	QLB.cpp 
	 */
	~QLB();
	
	// === Initialisation ===
	
	/** 
	 *	Allocates GPU memory needed during simulation
	 *	@file 	QLBcuda.cu	
	 */
	void allocate_device_arrays();
	
	/** 
	 *	Frees GPU memory used during simulation 	
	 *	@file 	QLBcuda.cu
	 */
	void free_device_arrays();
	
	/** 
	 *	init_device
	 *	@file 	QLBcuda.cu 
	 */
	void init_device();
	
	// === Initial condition ===
	
	/** 
	 *	Inital condition in which the positive energy, spin-up component is 
	 *	a spherically symmetric Gaussian wave packet with spread delta0.
	 * 	spinor0 = C * exp( -(x^2 + y^2) / (4 * delta0^2) )
	 *	@file 	QLB.cpp	
	 */
	void initial_condition_gaussian();

	// === Simulation ===
	
	/** 
	 *	Evolve the system for one time step 
	 *	@file	QLB.cpp	
	 */
	void evolution();
	void evolution_CPU();
	void evolution_GPU();

	/**
	 *	Construct the collision matrix Qhat by calculating X * Q * X^(-1) and
	 *	Y * Q * Y^(-1) respectively
	 *	@param 	i 	row index
	 *	@param 	j 	column index
	 *	@param 	Q 	collision matrix
	 */
	void Qhat_X(int i, int j, cmat_t& Q) const;
	void Qhat_Y(int i, int j, cmat_t& Q) const;

	/**
	 *	Caluclate the macroscopic variables
	 *	@file 	QLBcpu.cpp 
	 */
	void calculate_macroscopic_vars();	

	// === Potential ===
	
	/**
	 *	Harmonic potential: V(r) = 1/2 * m * w0^2 * r^2
	 *	@param i	row index
	 *	@param j 	column index 
	 */
	float_t V_harmonic(int i, int j) const;
	
	/**
	 *	Free-particle potential: V(r) = 0
	 *	@param i	row index
	 *	@param j 	column index 
	 */
	float_t V_free(int i, int j) const;

	// === Rendering ===

	/**
	 *	Setup all OpenGL context - this call is mandatory to use QLB::render()
	 *	@file 	QLBgraphics.cpp
	 */
	void init_GL();
	
	/** 
	 *	Render the current scene 
	 *	@file	QLBgraphics.cpp
	 */
	void render();
	
	/** 
	 *	Prepare the arrays and VBO's for rendering by copying the content of
	 *	desired simulation arrays (indicated by current_scene_) to the render
	 *  arrays (array_vertex_) and calculating the normals (array_normal_).
	 *	@file	QLBgraphics.cpp
	 */	
	void prepare_arrays();

	/** 
	 *	Draw a coordinate system (mainly used for debugging!)
	 *	@file	QLBgraphics.cpp
	 */	
	void draw_coordinate_system() const;
	
	// === IO ===

	/** 
	 *	get_device_arrays 
	 *	@param		
	 */
	void get_device_arrays();
	
	/** 
 	 *	Write the current content of the matrix or vector to STDOUT 
	 *	@param	m	matrix to be printed
	 *	@file 	QLB.cpp
	 */
	void print_matrix(const cmat_t& m) const;
	void print_matrix(const fmat_t& m) const;
		
	/** 
 	 *	Write the current content of the k-th vector element in the 
	 *	matrix (e.g spinor_) to STDOUT 
	 *	@param	m	matrix to be printed
	 *	@param  k 	index of vector in [0,4)
	 *	@file 	QLB.cpp 
	 */
	void print_matrix(const c4mat_t& m, std::size_t k) const;
	

	/** 
 	 *	Write the current content of all specified quantities (entries of 
 	 *	plot array) to the corresponding file. 
 	 *	Note: consecutive calls of this function will override the last content
 	 *		  of the files
	 *	@file 	QLB.cpp 
	 */
	void write_content_to_file();
	
	
	/** 
	 *	Adjust the scaling of the rendered scene
	 *	@param 	change_scaling	-1: decrese by factor of 2.0
	 * 	                         1: increse by factor of 2.0
	 */
	inline void change_scaling(int change_scaling) 
	{ 
		if(change_scaling == 1) scaling_ *= 2.0;
		else if(change_scaling == -1) scaling_ /= 2.0; 
	}
	
	// === Getter ===
	inline unsigned L() const { return L_;  }
	inline float_t dx() const { return dx_; }
	inline float_t mass() const { return mass_; }
	inline float_t t() const { return t_; }
	inline float_t dt() const { return dt_; }
	inline int V() const { return V_indx_; }
	inline bool verbose() const { return verbose_; }
	inline bvec_t plot() const { return plot_; }
	inline float_t deltax() const { return deltax_; }
	inline float_t deltay() const { return deltay_; }
	inline float_t scaling() const { return scaling_;}
	inline scene_t current_scene() const { return current_scene_; }
	inline render_t current_render() const { return current_render_; }
	
	// === Setter ===
	inline void set_current_scene(scene_t cs)   { current_scene_ = cs; }
	inline void set_current_render(render_t cr) { current_render_ = cr; }

private:
	// === Simulation variables ===
	unsigned L_;
	float_t dx_;
	float_t mass_;
	
	float_t t_;
	float_t dt_;

	float_t deltax_;
	float_t deltay_;
	float_t delta0_;
	
	int V_indx_;
	
	// === Arrays CPU === 
	c4mat_t spinor_;
	c4mat_t spinoraux_;
	c4mat_t spinorrot_;
	cmat_t  currentX_;
	cmat_t  currentY_;
	cmat_t  veloX_;
	cmat_t  veloY_;
	cmat_t  wrot_;
	cmat_t  rho_;
	
	// === Arrays GPU === 
	complex_t* d_spinor_;
	complex_t* d_rho_, d_rho0_;

	// === OpenGL context ===
	bool GL_is_initialzed_;
	scene_t  current_scene_;
	render_t current_render_;
	float_t scaling_;
	
	ivec_t  array_index_; 
	fvec_t  array_vertex_;
	fvec_t  array_normal_;
	
	VBO vbo_vertex;
	Shader shader_;
	
	// === IO ===
	std::ofstream fout;
	bool verbose_;
	bvec_t plot_;
};

#endif /* QLB.hpp */