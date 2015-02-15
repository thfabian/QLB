#!/usr/bin/env sh
#	
# Quantum Lattice Boltzmann 
# (c) 2015 Fabian Thüring, ETH Zürich
#
# This script reproduces the figures from the paper [1] by passing the 
# appropriate command-line arguments to the program QLB.
#
# [1] Isotropy of three-dimensional quantum lattice Boltzmann schemes, 
#     P.J. Dellar, D. Lapitski, S. Palpacelli, S. Succi, 2011

print_help() 
{
	echo "usage: $0 [options]"
	echo ""
	echo "options:"
	echo "   --fig-1     Print the figure 1 (no potential)"
	echo "   --fig-4     Print the figure 4 (harmonic potential)"
	echo "   --help      Print this help statement"
	echo "   --exe=PATH  Set the path of the QLB executable (e.g --exe=./QLB)"
	echo "   --tmax=X    Set the parameter '--tmax' of QLB to 'X'"
	echo "   --plot      Plot the output with python (executes plot/plot_spread.py)"
	exit 1
}

exit_after_error()
{
	echo "$1"
	exit 1
}

QLB_exe=./QLB

print_fig_1=false
print_fig_4=false
tmax_arg="--tmax=257"
plot_arg=""
python_flags=""

# Handle command-line arguments
if [ "$#" =  "0" ] 
then
   print_help 
fi

for param in $*
	do case $param in
		"--fig-1")  print_fig_1=true ;
		            python_flags="$python_flags --no-potential" ;;
		"--fig-4")  print_fig_4=true ;;
		"--help")   print_help ;;
		"--plot")   plot_arg="--plot=spread" ;;
		"--exe="*)  QLB_exe="${param##*=}" ;;
		"--tmax="*) tmax_arg="$param" ;;
		*) 	print_help ;; 
	esac
done

# Check if the executable exists/is valid
$QLB_exe --version > /dev/null 2>&1
if [ "$?" != "0" ]; then
	exit_after_error "$0 : error : executable '$QLB_exe' is not valid"
fi

# Run the simulation
if [ "$print_fig_1" = "true" ]; then
	$QLB_exe --L=128 --dx=0.78125 --dt=0.78125 --mass=0.35 --V=free --verbose \
	--gui=none $tmax_arg $plot_arg
elif [ "$print_fig_4" = "true" ]; then
	$QLB_exe --L=128 --dx=1.5625 --dt=1.5625 --mass=0.1 --V=harmonic --verbose \
	--gui=none $tmax_arg $plot_arg
else
	exit_after_error "$0 : error : no figure specified try '$0 --help'"
fi

if [ "$plot_arg" != "" ]; then 
	python plot/plot_spread.py spread.dat $python_flags
fi