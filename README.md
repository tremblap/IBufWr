### IBufWr, an interpolating buffer writer
v.1 by Pierre Alexandre Tremblay (2018)

#### Description
This is a SuperCollider (https://supercollider.github.io/) porting of the bespoke Max (https://cycling74.com/products/max) object ipoke~ v4.1 (http://www.no-tv.org/MaxMSP/), which allows to write to the server buffers without leaving unfilled indices when writing faster than realtime.

#### Why was this utility needed
It was impossible to emulate the musical behaviour of bespoke resampling digital delay pedals in Max and SuperCollider. Now it is.

#### How to Install from binaries (SC 3.9 on Mac required)
Download the package and read the text file.

#### How to build from the source code
These instructions are modified from the amazingly streamlined UGen writing tutorial by snappizz (https://github.com/supercollider/example-plugins)
1. Download the SuperCollider source
2. Create build directory
  1. `cd` to the folder where IBufWr is
  2. create a `build` directory
  3. `cd` into it
3. Set the compiler options and build
  4. set the make parameters by typing `cmake -DSC_PATH=PATH_TO_SUPERCOLLIDER_SOURCE -DCMAKE_BUILD_TYPE=Release ..`
    5. for a MacOS fat binary add `-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"` - or for a personal copy the option `-DCMAKE_CXX_FLAGS=-march=native` will optimise as fast as the machine will go, at the cost of compatibility with other hardware.
  6. type `make`, it should compile.
7. once it is done, install the Ugen and its Declaration by moving them in the `release-packaging/IBufWr/classes` and 'release-packaging/IBufWr/plugins' respectively, then move the whole `IBufWr` folder in your `Extensions` folder.
  8. for MacOS this is where code signature and notarization has to happen...

#### Enjoy! Comments, suggestions and bug reports are welcome.

##### Acknowledgements
This port was made possible thanks to the FluCoMa project (http://www.flucoma.org/) funded by the European Research Council (https://erc.europa.eu/) under the European Union’s Horizon 2020 research and innovation programme (grant agreement No 725899)

Thanks to Gerard Roma and Owen Green for their input.
