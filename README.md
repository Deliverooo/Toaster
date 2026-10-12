# Toaster 3.0 (Triple T)

Toaster is a WIP game engine framework that can facilitate 3D rendering. Unlike previous iterations of Toaster,
Toaster-3.0 is designed to have a much less restrictive API, letting you control exactly what you want to do, while also
being easy to use.

<p float="left" width=100%> 
	<img src="https://github.com/Deliverooo/Toaster-2.0/blob/3.0/github/images/Excuse_me_Orbo.png" width=25%>
</p>

# Features

- Data-oriented GPU API instead of highly coupled OOP.
- Utilisation of Vulkan 1.4 Descriptor heaps and fully bindless rendering architecture, eliminating the need for
  pipelines and descriptor sets.
- Orbo

# Using / Installing

1. Have Windows. I do not plan on ever supporting Linux or macOS...
2. Make sure you have the latest possible Vulkan SDK installed with Version >= 1.4 -> https://vulkan.lunarg.com/sdk/home
   as well as the latest graphics drivers for your system.
3. Git clone with ```git clone https://github.com/Deliverooo/Toaster.git```

If you want to install Toaster as the SDK (you probably should), then run ``install.bat`` (This will take a while
because all dependencies are gathered Via fetchcontent). That will install the Toaster SDK in your
``C:/Program Files/ToasterSDK/`` directory. Then, to use the generated CMake targets, simply add the ``bin``
directory in the SDK to your **System** environment varible list.

# Planned Features

- Support for loading animations and maybe sound
- A physics API utilising Jolt
- Improvements to the TSM (Toaster Math) library to support SIMD without using DirectXMath (This will be painful)