.. Copyright Celeritas contributors: see top-level COPYRIGHT file for details
.. SPDX-License-Identifier: CC-BY-4.0

.. highlight:: console

Plug-ins
========

Celeritas can run as a plugin to different integrated frameworks.

.. _plugins_larsoft:

LArSoft for DUNE
----------------

LArSoft is an integral component of the DUNE simulation framework. Celeritas
builds the ``PDFullSimCeler`` module to process optical photons from
scintillation. It requires ROOT input file with ``art::Event``
``sim::SimEnergyDeposit``object data from the ``IonAndScint`` producer, exactly
as the current ``PDFastSimPAR`` module in LArSoft. The ``PDFullSimCeler`` module
enables replacing the map-based method for generating the
scintillation-to-detector response by a full Monte Carlo optical tracking.

Building Celeritas as a LArSoft extension requires the whole larsoft toolchain,
available on Fermilab's ``scisoftbuild01``. The environment script at
``env/scisoftbuild01.sh`` can be sourced at startup to define an
``apptatiner_fermilab`` function that launches the container needed to build
and run.

Once inside the apptainer, initialize the UPS packaging system and load LArSoft
and DUNE components:

.. sourcecode::

   $ . /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
   Setting up larsoft UPS area... /cvmfs/larsoft.opensciencegrid.org
   Setting up DUNE UPS area... /cvmfs/dune.opensciencegrid.org/products/dune/
   $ setup -B dunesw v10_14_01d00 -q e26:prof

Finally, load the module/library/FHICL paths provided by Celeritas:

.. sourcecode::

   $ eval $($SCRATCHDIR/build/celeritas-reldeb-orange/bin/larceler-env)
   Loaded Celeritas at .../build/celeritas-reldeb-orange

Then you should be able to include Celeritas components.

.. sourcecode:: none

   #include "PDFullSimCeler.fcl"
   #include "standard_g4_dune10kt_1x2x6.fcl"

   dunefd_pdfullsim_cpu: {
     @table::PDFullSimCeler
   }

   # Use Celeritas full sim configuration
   physics.producers.PDFastSim: @local::dunefd_pdfullsim_cpu


LArSoft example FHiCLs and analyzer module
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following FHiCL files are available for generating optical simulation
samples and analyzing the generated detector response.

- ``dune10kt_ionandscint.fcl``: A FHiCL file to generate the ``IonAndScint``
  data products needed for the optical simulation.
- ``dune10kt_opticalsim.fcl``: A FHiCL file to run the optical simulation,
  either with ``PDFastSimPAR``, or Celeritas (via ``PDFullSimCeler``).

``PDSimAna`` module
"""""""""""""""""""

The resulting output from ``dune10kt_opticalsim.fcl`` can be analyzed with the
``PDSimAna`` module, which reads ``OpdetBacktrackerRecord`` data products to
produce analysis plots. The FHiCL file ``PDSimAna.fcl`` configures the module,
and ``PDSimAna_run.fcl`` sets up the output file and execution.

DD4HEP
------

Documentation to be added later.
