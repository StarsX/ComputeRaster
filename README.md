# ComputeRaster
Real-time software rasterzier using compute shaders, including vertex processing stage (IA and vertex shaders), bin rasterization, tile rasterization (coarse rasterization), and pixel rasterization (fine rasterization, which calls the pixel shaders). The execution of the tile rasterization pass adaptively depends on the primitive areas accordingly. In bin rasterization pass, if the primitive area is greater then 4x4 tile sizes, the bin rasterization will be triggered; otherwise, the bin rasterization pass will directly output to the tile space instead, and skip processing the corresponding primitive in the tile rasterization pass.

![Bunny result](https://github.com/StarsX/ComputeRaster/blob/master/Doc/Images/Bunny.jpg "Bunny raterized rendering result")
![Venus result](https://github.com/StarsX/ComputeRaster/blob/master/Doc/Images/Venus.jpg "Venus raterized rendering result")

Hot keys:

[F1] show/hide FPS

[Space] pause/play animation
