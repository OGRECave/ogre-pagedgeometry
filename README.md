# Introduction {#mainpage}

Although the PagedGeometry engine is fairly simple and easy to use, there are some
advanced features that may be difficult to learn on you own. This API reference is here
for your convenience, to aid you in learning how to get the most out of the PagedGeometry
engine.

Every feature of the engine is covered here in detail, so you won't be left in the dark
about any aspect of PagedGeometry's use (however, some of the internal workings of the
engine are not documented in here - you'll have to refer to the source code comments
for that).

# What is PagedGeometry?
The PagedGeometry engine is an add-on to the <a href="https://www.ogre3d.org">OGRE
Graphics Engine</a>, which provides highly optimized methods for rendering massive amounts
of small meshes covering a possibly infinite area. This is especially well suited for dense
forests and outdoor scenes, with millions of trees, bushes, grass, rocks, etc., etc.

![](docs/PagedGeometryScreen1.jpg) Expansive jungle scene with 240,000 trees and animated vegetation

Paged geometry gives you many advantages over plain entities, the main one being speed:
With proper usage of detail levels, outdoor scenes managed by PagedGeometry can
be >100x faster than plain entities. Another advantage is that the geometry is paged; in
other words, only entities which are immediately needed (to be displayed) are loaded.
This allows you to expand the boundaries of your virtual world almost infinitely
(only limited by floating point precision), providing the player with a more realistically
scaled game area.

# Features
* Dynamic geometry paging system, which enables infinite worlds
* Batched rendering for optimized rendering of near-by trees
* Impostor rendering -LOD for extremely fast rendering of distant trees
* Flexible -LOD display system, which can be expanded to display geometry with any technique you can implement
* Flexible -LOD configuration system, which allows you to configure any combination of supported LODs in any way you want
* Optional cross-LOD fade transitions, and far -LOD fade-out, fully configurable
* Flexible PageLoader system, allowing you to dynamically load geometry from any source you can imagine
* Easy addition / removal of trees with bit packing, allowing millions of trees to be stored in RAM using only a few MBs
* Color-map support for trees, which enables you to apply terrain lightmaps to your trees with one simple function call
* Animated, optimized grass rendering system. Supports density maps, color maps, wind animations, height range restriction, and much more.

# Getting Started

When you're ready to start learning how to use PagedGeometry, the best place to start is
with @ref tut1. The tutorials will teach you how to use many
important PagedGeometry features, step by step. This API reference isn't recommended
for learning, but is a valuable resource when you need specific in-depth information
about a certain function or class.


# Credits

<ul>
<li><b>John Judnich</b> - <i>Programming / design / documentation</i></li>
<li><b>Alexander Shyrokov</b> (aka. sj) - <i>Testing / co-design</i></li>
<li><b>Tuan Kuranes</b> - <i>Imposter image render technique</i></li>
<li><b>(Falagard)</b> - <i>Camera-facing billboard vertex shader</i></li>
<li><b>Wendigo Studios</b> - <i>Tree animation code & various patches/improvements</i></li>
<li><b>Thomas Fischer</b> - <i>Maintainer from Jun/2009</i></li>
</ul>


# License
<b>Copyright (c) 2007 John Judnich</b>

<i>
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

<b>1.</b> The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

<b>2.</b> Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

<b>3.</b> This notice may not be removed or altered from any source distribution.
</i>