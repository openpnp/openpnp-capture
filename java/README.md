Tested so far with a Mac library created in Xcode as a Cocoa dyamic library. Right clicked
the product in Xcode, opened in Finder, copied path and made a symlink from target/classes/darwin
to that directory. 

darwin -> /Users/jason/Library/Developer/Xcode/DerivedData/openpnp-capture-ffimtldhkvzsplcccuuggkxxxqoe/Build/Products/Debug

Updated Run Configuration to use -Djna.library.path=/Users/jason/Library/Developer/Xcode/DerivedData/openpnp-capture-ffimtldhkvzsplcccuuggkxxxqoe/Build/Products/Debug.

Looks like we can just bundle the jars and all will work well.

Library Search Paths A search for a given library will scan the following locations:

jna.library.path User-customizable path
jna.platform.library.path Platform-specific paths
On OSX, ~/Library/Frameworks, /Library/Frameworks, and /System/Library/Frameworks will be searched for a framework with a name corresponding to that requested. Absolute paths to frameworks are also accepted, either ending at the framework name (sans ".framework") or the full path to the framework shared library (e.g. CoreServices.framework/CoreServices).
Context class loader classpath. Deployed native libraries may be installed on the classpath under ${os-prefix}/LIBRARY_FILENAME, where ${os-prefix} is the OS/Arch prefix returned by Platform.getNativeLibraryResourcePrefix(). If bundled in a jar file, the resource will be extracted to jna.tmpdir for loading, and later removed (but only if jna.nounpack is false or not set).
You may set the system property jna.debug_load=true to make JNA print the steps of its library search to the console.


java -jar jnaerator/target/jnaerator-0.13-SNAPSHOT-shaded.jar -library openpnp-capture ~/Projects/openpnp/openpnp-capture/include/openpnp-capture.h -o ~/Desktop -mode Directory -f -runtime JNA