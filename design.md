# Public Design Document for Viper
This document describes all planned and implemented features of the Viper software. Changes can be suggested through pull requests and issues. We use this document to communicate the larger intent, because we're designing a production software together, and everyone is involved in that process.

It can be assumed that features appearing first are to be implemented or fixed first.

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
