### IBufWr, an interpolating buffer writer
v.1 by Pierre Alexandre Tremblay (2018)

#### Description
This is a SuperCollider (https://supercollider.github.io/) porting of the bespoke Max (https://cycling74.com/products/max) object ipoke~ v4.1 (http://www.no-tv.org/MaxMSP/), which allows to write to the server buffers without leaving unfilled indices when writing faster than realtime.

#### Why was this utility needed
It was impossible to emulate the musical behaviour of bespoke resampling digital delay pedals in Max and SuperCollider. Now it is.

#### How to Install from binaries (SC 3.9 on Mac required)
1. If you read this, you must have downloaded the binary package. If not, download it from the GitHub repository.
2. Drag the full `IBufWr` folder, with its 3 subfolders (classes, HelpSource, plugins) in your `Extensions` folder. If you don't know what this is, please read the SuperCollider instructions here: (http://doc.sccode.org/Guides/UsingExtensions.html)
3. Enjoy!

#### How to get started
The helpfile gives 3 typical usage of the UGen, and there is a further document that gives more details on the justification, implementation, and example code.

Comments, suggestions and bug reports are welcome.

##### Acknowledgements
This port was made possible thanks to the FluCoMa project (http://www.flucoma.org/) funded by the European Research Council (https://erc.europa.eu/) under the European Union’s Horizon 2020 research and innovation programme (grant agreement No 725899)

Thanks to Gerard Roma and Owen Green for their input.
