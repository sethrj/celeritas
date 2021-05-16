#----------------------------------*-CMake-*----------------------------------#
# Copyright 2020 UT-Battelle, LLC and other Celeritas Developers.
# See the top-level COPYRIGHT file for details.
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
#[=======================================================================[.rst:

CeleritasUtils
--------------

CMake utility functions for Celeritas.

.. command:: celeritas_option

  ::

    celeritas_option(<short_variable> "<description>" <value>)

  Define an option named ``CELERITAS_<short_variable>`` that defaults to the
  value of ``<short_variable>``. This is designed to simplify user configuration
  scripts while allowing the CMake project to play nicely in the context of a
  larger build system.

.. command:: celeritas_find_package_config

  ::

    celeritas_find_package_config(<package> [...])

  Find the given package specified by a config file, but print location and
  version information while loading. A well-behaved package Config.cmake file
  should already include this, but several of the HEP packages (ROOT, Geant4,
  VecGeom do not, so this helps debug system configuration issues.

  The "Found" message should only display the first time a package is found and
  should be silent on subsequent CMake reconfigures.

  Once upstream packages are updated, this can be replaced by ``find_package``.


.. command:: celeritas_link_vecgeom_cuda

  Link the given target privately against VecGeom with CUDA support.

  ::

    celeritas_link_vecgeom_cuda(<target>)

#]=======================================================================]
include(FindPackageHandleStandardArgs)

function(celeritas_option var description default)
  if(DEFINED ${var})
    # Override the *default* based on the parent value. If the prefixed variable
    # name is already set, then the short variable has no effect.
    set(default ${${var}})
  endif()
  option(CELERITAS_${var} "${description}" "${default}")
endfunction()

macro(celeritas_find_package_config _package)
  find_package(${_package} CONFIG ${ARGN})
  find_package_handle_standard_args(${_package} CONFIG_MODE)
endmacro()

function(celeritas_link_vecgeom_cuda target)
  set_target_properties(${target} PROPERTIES
    LINKER_LANGUAGE CUDA
    CUDA_SEPARABLE_COMPILATION ON
  )
  target_link_libraries(${target}
    PRIVATE
    VecGeom::vecgeomcuda
    VecGeom::vecgeomcuda_static
  )
endfunction()

#-----------------------------------------------------------------------------#
