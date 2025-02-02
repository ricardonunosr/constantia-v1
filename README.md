# Constantia

This project is my personal general 3D engine to learn graphics and game engine
related stuff. This project is originated from my first project in 3D engine
development named [Valiance](https://github.com/ricardonunosr/valiance) that was
very much copied/inspired from [Hazel](https://github.com/TheCherno/Hazel) and i
wanted to start fresh and relearn the basics.

Requeriments

- [CMake](https://cmake.org/)

Dependencies

- [glfw](https://github.com/glfw/glfw)
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)
- [glad2](https://github.com/Dav1dde/glad/tree/glad2)
- [imgui](https://github.com/ocornut/imgui)
- [stb](https://github.com/nothings/stb)

# Screenshots

![cube-screenshot](./data/cube-screenshot.png)
![sponza-screenshot](./data/sponza-screenshot.png)

# Getting started

- Clone the repo

```bash
git clone --recursive https://github.com/ricardonunosr/constantia
```

- If you cloned non-recursively

```bash
git submodule update --init
```

- Change to the directory

```bash
cd constantia
```

- Use CMake to build repo

```bash
mkdir build
cd build
cmake ..
make
```

```bash
sudo dtrace -x stackframes=100 -n 'profile-997 /arg0/ { @[ustack()] = count(); } tick-60s { exit(0); }' -o out.perf -c ./build/Constantia
stackcollapse.pl out.perf > out.perf_folded
flamegraph.pl out.perf_folded > profile.svg
```
