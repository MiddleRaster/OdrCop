# OdrCop

A tool to detect ODR violations, based on DIA so it only works with binaries build with the MSVC toolset.

Usage: odrcop <a.pdb> <b.pdb> [c.pdb ...] where each .cpp file/TU is built with /Zi and /Fd"x64\Debug\%(Filename).pdb"
