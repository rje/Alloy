# Alloy

Alloy is a project to build a standalone, open source, modifiable CAD application
that grows into a functional feature-for-feature replacement for the reference CAD application.
Existing reference CAD application users should find its workflows, commands, keyboard controls, and layout
intimately familiar. Features that currently require cloud access may be deferred.

This is a clean-room implementation based on public documentation, instructions,
and screenshots. We do not run or inspect reference CAD application. Read [AGENTS.md](AGENTS.md)
before researching, designing, or implementing anything in this project.

The first product focus is shape creation: sketches, solids, surfaces, editable
history, common-format import, and export for 3D printing. The initial scaffold
is an application shell, not a functioning CAD modeler. Agent control and
self-testing are part of the application from its first runnable version.

The public repository is [rje/Alloy](https://github.com/rje/Alloy), with the
[issue backlog](https://github.com/rje/Alloy/issues) and
[Alloy project board](https://github.com/users/rje/projects/1).

## Project documents

- [Continuation context](docs/context.md): decisions from this conversation,
  current implementation, evidence, and where to resume.
- [Work backlog](docs/backlog/README.md): researched tasks, dependencies,
  cumulative validation gates, and the live pull queue.
- [Deep-dive research](docs/research/cad-workflows.md): public workflow
  findings behind the backlog, with a dated source registry.
- [Product scope](docs/product-scope.md): accepted goals, initial coverage, and
  the longer-term feature families.
- [Architecture](docs/architecture.md): recommended stack, document model,
  geometry boundaries, and unresolved choices.
- [Roadmap](docs/roadmap.md): workflow milestones and acceptance criteria.
- [Compatibility](docs/compatibility.md): how we track familiar behavior and
  separate research evidence from implementation claims.
- [Target comparison](docs/target-comparison.md): the Maya, Houdini, and reference CAD application
  paths considered before selecting reference CAD application.
- [Public interface research](docs/research/interface-reference.md): Astra's
  source-linked interface and interaction findings.
- [Development](docs/development.md): build, run, and verification instructions.
- [Agent control](docs/agent-control.md): local command protocol and client.

## Starting decisions

C++20 is the selected application language. Qt 6 and Open CASCADE Technology
(OCCT) are the recommended foundation.
Python is suitable for the initial automation client. Solver selection, native
document format, and dependency distribution choices still need decisions.
Rust is outside the planned stack.

Alloy's original source and documentation use the [MIT license](LICENSE).
Third-party libraries and public reference material retain their own terms.

The 3D viewport must be GPU accelerated, with live renderer diagnostics and
native framebuffer capture verified. The original QPainter grid is a temporary
shell baseline.

To list tasks whose dependencies are complete:

```sh
python3 tools/backlog.py ready
```

GitHub operations use `gh` and documented APIs.

Alloy is the working project name. Public naming and trademark clearance have
not been performed. There is no affiliation with Autodesk.
