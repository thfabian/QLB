@echo off
REM Quantum Lattice Boltzmann 
REM (c) 2015 Fabian Thüring, ETH Zürich
REM
REM This script launches the 64-bit Windows version in GUI mode

cd %~dp0/bin/Windows/64-bit
start QLBwin.exe --L=512 --start-rotating
