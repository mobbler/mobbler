languages.pl %1 %2
pushd languages
mkdir unsigned
for /f %%a IN ('dir /b unsigned\*.pkg') do call makesis unsigned\%%a
for /f %%a IN ('dir /b unsigned\*.sis') do call signsis unsigned\%%a %%aX ../mobbler.cer ../mobbler.key
del /q unsigned\*.*
rmdir unsigned
popd
