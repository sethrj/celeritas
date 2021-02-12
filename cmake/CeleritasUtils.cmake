#----------------------------------*-CMake-*----------------------------------#
# Copyright 2020 UT-Battelle, LLC and other Celeritas Developers.
# See the top-level COPYRIGHT file for details.
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
#[=======================================================================[.rst:

CeleritasUtils
--------------

CMake utility functions for Celeritas.

.. command:: celeritas_check_python_module

   Determine whether a given Python module is available with the current
   environment. ::

     celeritas_check_python_module(<variable> <module>)

   ``<variable>``
     Variable name that will be set to whether the module exists

   ``<module>``
     Python module name, e.g. "numpy" or "scipy.linalg"

   Note that because this function caches the Python script result to save
   reconfigure time (or when multiple scripts check for the same module),
   changing the Python executable or installed modules may mean
   having to delete or modify your CMakeCache.txt file.

   Example::

      celeritas_check_python_module(has_numpy "numpy")

#]=======================================================================]

function(celeritas_check_python_module result_var module_name)
  set(_cache_name CELERITAS_CHECK_PYTHON_MODULE_${module_name})
  if(DEFINED ${_cache_name})
    # We've already checked for this module
    set(_found "${${_cache_name}}")
  else()
    message(STATUS "Check Python module ${module_name}")
    set(_cmd "${Python_EXECUTABLE}" -c "import ${module_name}")
    execute_process(COMMAND
      ${_cmd}
      RESULT_VARIABLE _result
      ERROR_QUIET # hide error message if module unavailable
    )
    if(_result)
      set(_msg "not found")
      set(_found FALSE)
    else()
      set(_msg "found")
      set(_found TRUE)
    endif()
    message(STATUS "Check Python module ${module_name} -- ${_msg}")
    set(${_cache_name} "${_found}" CACHE INTERNAL
      "Whether Python module ${module_name} is available")
  endif()

  # Save outgoing variable
  set(${result_var} "${_found}" PARENT_SCOPE)
endfunction()

#-----------------------------------------------------------------------------#
