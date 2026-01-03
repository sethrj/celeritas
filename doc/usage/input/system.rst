.. Copyright Celeritas contributors: see top-level COPYRIGHT file for details
.. SPDX-License-Identifier: CC-BY-4.0

.. _inp_system:

System
======

System options configure low-level execution parameters such as MPI, GPU
capabilities, logging, and performance profiling. These are typically set up
once per program execution and are not loaded by the :cpp:struct:`celeritas::inp::Problem`
definition but are used by standalone/framework inputs.

.. celerstruct:: inp::System

MPI Configuration
-----------------

Control whether MPI is enabled for parallel execution.

.. doxygentypedef:: celeritas::inp::DefaultMpi
.. celerstruct:: inp::DisableMpi
.. celerstruct:: inp::EnableMpi

.. doxygentypedef:: celeritas::inp::Mpi

Execution Mode
--------------

Choose where Celeritas will execute: CPU or GPU.

.. doxygentypedef:: celeritas::inp::DefaultExecution
.. celerstruct:: inp::DisableExecution
.. celerstruct:: inp::NullExecution
.. celerstruct:: inp::CpuExecution
.. celerstruct:: inp::GpuExecution

.. doxygentypedef:: celeritas::inp::Execution

Logger Configuration
--------------------

Control verbosity levels for different logging outputs.

.. celerstruct:: inp::Logger

Performance Profiling
---------------------

Enable performance profiling for CPU or GPU execution.

.. doxygentypedef:: celeritas::inp::DefaultProfiling
.. celerstruct:: inp::DisableProfiling
.. celerstruct:: inp::PerfettoProfiling
.. celerstruct:: inp::DeviceProfiling

.. doxygentypedef:: celeritas::inp::Profiling

Environment Variables
---------------------

The ``environment`` field in ``System`` is a map of environment variables that
can be set programmatically. These variables are merged into the process
environment during system setup. Note that attempting to set an environment
variable that has already been used will result in an error.
