# Toaster 3.0

Toaster is a work-in-progress game/graphics engine **framework** that can facilitate 3D rendering. Unlike previous
iterations of Toaster,
Toaster-3.0 is designed to have a much less restrictive API, letting you control exactly what you want to do, while also
being easy to use. This is because it is designed in a C-style C++ way that prioritises data locality and cache
efficiency over heavy OOP classes and inheritance, thereby decoupling itself

<p float="left" width=100%> 
	<img src="https://github.com/Deliverooo/Toaster-2.0/blob/3.0/github/images/Excuse_me_Orbo.png" width=25%>
</p>

# Features

- Data-oriented GPU API instead of highly coupled OOP.
- Aimed at being very CPU cache-friendly, so a lot of operations are able to be executed in batches
- Utilisation of Vulkan 1.4 Descriptor heaps and fully bindless rendering architecture, eliminating the need for
  pipelines and descriptor sets.
- Orbo

# Using / Installing

1. Have Windows. I do not plan on ever supporting Linux or macOS...
2. In previous versions, you would have needed the Vulkan SDK. However, due to Toaster dynamically loading to Vulkan at
   runtime Via Volk, you technically don't need it. Although, I believe that validation layers may not work without it.
   Importantly, your Vulkan  
   version has to be >= 1.4 -> https://vulkan.lunarg.com/sdk/home
   as well as having the latest graphics drivers for your system (mostly because of vk_EXT_descriptor_heap)
3. Git clone with ```git clone https://github.com/Deliverooo/Toaster.git```

If you want to install Toaster as the SDK (you probably should), then run ``install.bat`` (This will take a while
because all dependencies are gathered Via fetchcontent). That will install the Toaster SDK in your
``C:/Program Files/ToasterSDK/`` directory. Then, to use the generated CMake targets, simply add the ``bin``
directory in the SDK to your **System** environment varible list.

From here, you can call `find_package(Toaster CONFIG
REQUIRED)` in your CMake projects and use Toaster's subsystems (in CMake the targets are: `Toaster::ToastMath`, `Toaster::
ToastOS`, `Toaster::ToastLib`, `Toaster::ToastGPU`, `Toaster::ToastRender`, `Toaster::ToastKernel` and
`Toaster::ToastAsset`)

# Planned Features

- Even more work on the GPU API to make it essentially a whole library ecosystem
- Possible integration of KTX2 textures and GLTF models/animations to achieve even more *peak* performance
- Improvements to the TSM (Toaster Math) library to support SIMD without using DirectXMath (I don't like Microsoft's
  APIs)