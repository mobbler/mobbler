# Introduction

Mobbler is an open source project.  Please feel free to checkout the source and build it yourself.  Instructions below.

If there is something you want to change, please change it.  I would love to include your improvements in the project.

If you raise an issue on the issues tab and attach your changes, I will try to include them as soon as possible.  If it's a large change or something that might be controversial, please raise an issue first so that we can discuss it before putting any work in.

I may consider adding you as a project member after making a few changes (not really sure on the criteria yet) so that you will be able to submit changes yourself.

# Build Instructions

## Get the Mobbler source code

Please go to [this page](http://code.google.com/p/mobbler/source/checkout) for instructions on how to check out Mobbler using Mercurial.

I recommend that Windows users use [TortoiseHG](http://bitbucket.org/tortoisehg/stable/wiki/Home) which is a lovely Windows Explorer plugin that makes it easy to do pretty much everything you would want to do with Mercurial. It's not as polished as TortoiseSVN yet though.

## Install Carbide

If you are reading this then you probably already have Carbide installed, but if not, you can go to the [Symbian tools and kits page](http://developer.symbian.org/main/tools_and_kits/) to download and install the latest version of the Application Development Kit (ADT) which includes Carbide.  It's an IDE based on Eclipse that you will do your developing in.

## Get a Symbian SDK

Download and install a Symbian SDK linked to from [here](http://developer.symbian.org/main/tools_and_kits/index.php).  You can also use the 3rd edition Feature Pack 2 SDK, if you prefer, which can be found [here](http://www.forum.nokia.com/info/sw.nokia.com/id/ec866fab-4b76-49f6-b5a5-af0631419e9c/S60_All_in_One_SDKs.html).

## Install S60 SDK API Plug-ins

To do a complete build of Mobbler you will need a few Nokia SDK API Plug-ins.  If you just want to build a basic version of Mobbler, without music player observing and some other features, you can skip to the next step.

The page containing the main SDK API Plug-ins can be found [here](http://wiki.forum.nokia.com/index.php/SDK_API_Plug-in).

The plug-ins you will need for Mobbler are the [Browser Launcher API](http://wiki.forum.nokia.com/index.php/Browser_Launcher_API), [Music Player Remote Control API](http://wiki.forum.nokia.com/index.php/Music_Player_Remote_Control_API), [SW Installer Launcher API](http://wiki.forum.nokia.com/index.php/SW_Installer_Launcher_API) and the [Audio Metadata Reader API](http://wiki.forum.nokia.com/index.php/Audio_Metadata_Reader_API) which are all included in the zip file [here](http://www.forum.nokia.com/info/sw.nokia.com/id/4ff42a22-7099-4cc9-91bf-5e66166bd28d/S60_3rd_SDK_FP1_API_Plug-In_Pack.html).  You will need to install the 5th edition SDK extension bundle for the MPX APIs found [here](http://www.forum.nokia.com/info/sw.nokia.com/id/48a93bd5-028a-4b3e-a0b1-148ff203b2b3/Extensions_plugin_S60_3rd_ed.html).  You will also need the [Nokia Sensor APIs](http://wiki.forum.nokia.com/index.php/Nokia_Sensor_APIs) from [here](http://www.forum.nokia.com/info/sw.nokia.com/id/4284ae69-d37a-4319-bdf0-d4acdab39700/Sensor_plugin_S60_3rd_ed.html).

## Build Mobbler

## Without plugins

Without the SDK plugins you will only be able to build the main Mobbler executable.

If you are using Carbide, you should import the project, but untick everything except for mobbler.mmp and icons.mk.  You can then build it as usual.

You can also build on the command line using the normal build tools as below.

```
cd group
bldmake bldfiles
abld build icons
abld build winscw udeb mobbler
abld build gcce urel mobbler
```

## With plugins

This should just be the normal Symbian build process.

If you are using Carbide, you should just import the project and build it as usual.

You can also build on the command line using the normal build tools as below.

```
cd group
bldmake bldfiles
abld build winscw udeb
abld build gcce urel 
```

## Create the .SIS file

The mobbler.pkg file included in the Mobbler source is written to be used within Carbide.  You can get Carbide to build the .SIS for you by adding it in the project settings.  Please choose to self sign the .SIS file and select the mobbler.cer and mobbler.key provided.  The mobbler.pkg is also written to include all the plugins, if you are building without the plugins and want to install to your device, you can comment out the line that include the plugins.
