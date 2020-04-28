# Public Design Document for Viper
This document describes all planned and implemented features of the Viper software. Changes can be suggested through pull requests and issues. We use this document to communicate the larger intent, because we're designing a production software together, and everyone is involved in that process.

It can be assumed that features appearing first are to be implemented or fixed first.

## Table of Contents
 - [Projects](#Projects)
   * [Resources](#Resources)
   * [Format](#Format)
 - [Rendering](#Rendering)
   * [Backend Drivers](#Backend-Drivers-"vulkan_driver.h",-etc.)
   * [Rendering API](#Rendering-API-"rendering.h")
   * [High-level GUI](#High-level-GUI-"gui.h")

## Projects

A user must create a new project to begin adding audio and video clips, or composing video. The project exists as a single file (a ".viper") which includes all of the project-specific information, like which audio and video clips are being used, how they're cut, aligned, sequenced, any effects, and more.

### Resources
Resources are user-created and user-provided data, like video and audio files, that a project relies on. A project keeps a single reference to a single resource. When a resource cannot be found, we search for it in the same folder as the project file.

### Format
Many commercial video editors, including Vegas Pro by MAGIX, use binary formats for their project files that cannot be modified or read without the commercial editor. Using a text format similar to the language used in Godot data files would be supremely beneficial to both users and developers, for its simplicity.

An example project file, with minimal settings:
```
[Settings]
title="Untitled Project"

[Resources]
id1=Resource("local-dir/video-file.mp4")

[Tracks]
video-track-1=[
    VideoSegment(id1, 0, 0),  # Loaded clip of resource "id1", in the track at frame zero,
                              # starting at frame zero in the clip.
]
audio-track-1=[
    AudioSegment(id1, 0, 0),  # Same configuration as above
]
```

## Rendering

Vulkan is the primary backend Viper targets. All other backends are optional. We want to have a rendering pipeline and API that allows for swapping out backends. Further, a high-level GUI API will be programmed as an abstraction to the rendering API, to allow for easily constructing user-interface components, and making look and feel universal.

### Backend Drivers "vulkan_driver.h", etc.
A backend driver must be written to support a backend. The backend driver meets a specification of functions and values by the Driver interface in driver.h. The interface provides an initialization and cleanup function to backends.

### Rendering API "rendering.h"
The rendering API provides a consistent, portable API for programming graphics in Viper. This API will be either directly or indirectly used for all graphics rendering in the software. The rendering API communicates to a graphics library driver for anything needing to render.

Only one driver can be loaded at runtime, but the backend driver could be swapped out using a config entry change between software restarts.

### High-level GUI "gui.h"
All of Viper's reusable GUI components will be programmed in this API. Most of Viper's visual presentation will use this API. Anything that doesn't is aiming to use lower-level control for more specific rendering, such as visualizations, effects, and other graphics which a GUI library isn't concerned about.
