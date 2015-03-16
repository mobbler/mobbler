# Introduction

These are instructions for _developers_ how to add a new or update an existing language to Mobbler.

Instructions for _translators_ are [here](Language.md).


# Add a new language

  1. Check the Enum and PKG lang-code from http://wiki.forum.nokia.com/index.php/TLanguage_enumeration
  1. Name the file mobbler.lXX where XX is enum.
  1. Make sure the file is encoded in UTF-8 without BOM, especially if it has "special" non-ASCII characters/accents.
  1. Include mobbler.lXX in mobbler.loc.
  1. Add the enum to the LANG list in mobbler.mmp.
  1. Make sure it builds.
  1. In mobbler.pkg and mobbler\_signed.pkg:
    * Add the PKG lang-code; it must be in the same order as its corresponding enum.
    * Also add an extra "Mobbler" entry in each of those two lines, and an extra "Series60ProductID" in those other two lines.
    * Add new mobbler\_0xA0007648.rXX entries for both the resource file and language localisation file.
  1. Check you can create a sisx file.
  1. In languages.csv, add the enum, PKG lang-code, language name (use underscores instead of spaces), and ISO 639-1 language code.
  1. Make a language patch: make sure EPOCROOT is set and run "languages 0.x.y" to make one for every language in languages.csv. (Tip: just include the new language in the csv to save time, but restore the full list later.)
  1. Optionally check the patch installs and/or send the patch to the translator for testing and updating before releasing.
  1. Commit the new file and modification to source control.
  1. Edit the languages wiki page to include the new language.
  1. Upload the language patch file using googlecode\_upload\_1.bat.
  1. Tweet it!

# Update an existing language

  1. Merge the new translation into the existing one.
  1. Make sure it builds.
  1. Edit languages.csv to include only the new language and a newline (revert the file later).
  1. Make a language patch: make sure EPOCROOT is set and run "languages 0.x.y". (Suggestion: Start from 0.x.0 for patches sent to translator for alpha testing, and start from 0.x.10 for publicly released patches, where x matches the corresponding Mobbler release.)
  1. Optionally check the patch installs and/or send the patch to the translator for testing and updating before releasing.
  1. Commit the new file and modification to source control.
  1. Update the languages wiki page.
  1. Upload the language patch file using googlecode\_upload\_1.bat.
  1. Set the previous language patch to Deprecated.