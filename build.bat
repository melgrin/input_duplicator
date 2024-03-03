@echo off

if not exist bin mkdir bin
pushd bin

@echo on
:: cl -nologo -MT -Gm- -GR- -EHa- -Od -Oi -W4 -WX -wd4201 -wd4189 -wd4100 -Z7 %* -Fe:input_duplicator ..\src\main.c /link /manifest:embed -opt:ref user32.lib gdi32.lib winmm.lib
cl -nologo -MT -Gm- -GR- -EHa- -Od -Oi -W4 -WX -wd4201 -wd4189 -wd4100 -Z7 %* -Fe:input_duplicator ..\src\main.c /link /manifest:embed
@echo off

popd

:: warning C4189: local variable is initialized but not referenced
:: warning C4100: unreferenced formal parameter
