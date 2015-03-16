# Introduction

SVGs such as those found in the [Tango Desktop Project](http://tango.freedesktop.org/) need to be modified before they can be used in S60.

According to the author of the [Tango Symbian60 Theme](http://wiki.sukimashita.com/Tango_Symbian60_Theme):

> The The basic issue here is that the phone's SVG implementation is based on the SVG Mobile 1.2 specification. Thus it is only able to interpret a specific subset of the SVG XML.

> For the Tango icons the main issue arises due to use of gradient stroke which SVG Mobile does not support.

> You can use the free SVG editor http://www.inkscape.org/ to edit those and convert the gradients to regular fill gradients or even simply make them have a plain color (people won't notice the difference on the screen).

> The display/rendering artifacts should be gone then.


# Instructions

  1. Open the SVG in a text editor and add an attribute viewBox="0 0 48 48" to the svg element. Once added, Inkscape retains it.
  1. Open the SVG in Inkscape.
  1. Save as "Plain SVG".
  1. Give it a try in the emulator. If any parts don't show up, you will need to change gradient strokes to gradient fills or flat colours. (You may need to Object -> Ungroup.)
  1. Right click -> Fill and Stroke, monkey about with the settings, save and try it out in emulator.
  1. I think it also doesn't like "ellipse" shapes, so try and replace those with "path" shapes.
  1. When ready, File -> Vacuum Defs, will remove some unused guff to reduce the size by around 55%.
  1. (Finally, you can also run it through an SVG to SVG-T converter to make them a bit smaller. TODO which converter and how?)



# Links

  * [Inkscape SVG editor](http://www.inkscape.org/)
  * [Tango Desktop Project](http://tango.freedesktop.org/)
  * [Tango Symbian60 Theme](http://wiki.sukimashita.com/Tango_Symbian60_Theme)
  * [Issue 193](https://code.google.com/p/mobbler/issues/detail?id=193): Use the Tango Icon Library for Mobbler graphics