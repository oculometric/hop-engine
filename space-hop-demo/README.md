# Template Project
this is a template project for hop-engine. it won't build out of the box, you need to download a hop-engine release (or build it yourself), and point the `HOP_ENGINE` variable in CMakeLists.txt to the root directory of the library.

there are two targets - `build`, which builds the project, and `export`, which copies the built resources necessary to run `build` to another directoy (for sharing your project).

from here you can add your own code in `src`, and your own game assets in `res`. there is some example code already!

most importantly, have fun, and remember to do your bunny best.