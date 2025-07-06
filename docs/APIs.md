# Nexus APIs

This document provides an overview of the different APIs available in the Nexus framework. There are 2 User APIs (Core C++ and Python) and 2 Vendor APIs (Plugin and JSON).

## Table of Contents

- [Quick Start](Quick_Start.md) - Get up and running quickly
- [Core API](Core_API.md) - C++ API for high-performance applications
- [Python API](Python_API.md) - Python bindings for cross-platform hardware accelerator programming
- [Plugin API](Plugin_API.md) - C API for implementing hardware backend plugins
- [JSON API](JSON_API.md) - JSON-based device information and property system
- [Build and CI](Build_and_CI.md) - Build instructions and continuous integration setup

## Overview

Nexus provides multiple API layers to support different use cases:

- **Core API**: C++ API for high-performance applications and system integration
- **Python API**: High-level, object-oriented interface for rapid development and prototyping
- **Plugin API**: Low-level C API for implementing new hardware backends and runtime plugins
- **JSON API**: JSON-based interface for device information and property queries

Each API is documented in its own file with detailed examples and reference information.

For build instructions, continuous integration setup, and development workflow, see the [Build and CI](Build_and_CI.md) documentation. 