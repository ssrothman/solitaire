---
name: project-conventions
description: "Workspace rules for the solitaire project: always use the dev conda environment for local commands, and mark private variables and methods with leading underscores."
applyTo: "**"
---

# Project Conventions

- Always run local build, test, and validation commands in the `dev` conda environment, for example with `conda run -n dev ...`.
- Mark private variables and methods with leading underscores.
- Treat these as hard project conventions when editing or adding code.
