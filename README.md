<p align="center">
  <img src="static/logo.png" alt="Sublime's custom image"/>
</p>

# Bonfire
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
<br/>

Bonfire is a video-conferencing platform for those who miss the good old Paleolithic days.

## Goals
* Optimal client resources utilization: server-side audiomixing and videoscaling on demand
* Scaling to thousands of participants
* Easy integration of custom media-processing

## Roadmap
The project is in active development, you can refer to the [roadmap](https://github.com/kisasexypantera94/bonfire/issues/1) for the current status.

## Setup
Start service:
```console
$ docker compose up --build --remove-orphans --force-recreate
```

## References
[VK Calls Architecture](https://habr.com/ru/company/vk/blog/575358/) – Bonfire is heavily inspired by this blog series, read it if you want to get a general idea of how it works
