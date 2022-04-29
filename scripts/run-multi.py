#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2022 UT-Battelle, LLC, and other Celeritas developers.
# See the top-level COPYRIGHT file for details.
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
"""

Inputs:

- Outer loop count, inner parallel launch
- Launcher arguments
- Output directory/prefix

In-flight:

- Forward SIGINT, SIGUSR2 to processes
- Redirect stderr to file
- Save stderr to pipe
- SIGTERM/SIGKILL after waiting

Output per run:

- Reindent JSON output
"""

