.. Copyright Celeritas contributors: see top-level COPYRIGHT file for details
.. SPDX-License-Identifier: CC-BY-4.0

.. include:: ../../example/data-model/README.rst

Core infrastructure example
===========================

This simple example shows how to incorporate an already-installed Celeritas
into a downstream project and use the Collection data structures.

CMake infrastructure
--------------------

The CMake code itself is straightforward, though note the use of
``celeritas_target_link_libraries`` instead of ``target_link_libraries`` to
support CUDA RDC, which is required by VecGeom.

.. literalinclude:: CMakeLists.txt
   :language: cmake
   :start-at: project(

Main executable
---------------

.. literalinclude:: data-model.cc
   :start-at: #include
