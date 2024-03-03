@echo off

if not exist bin mkdir bin
pushd bin

@echo on
cl -nologo -MT -Gm- -GR- -EHa- -Od -Oi -W4 -WX -wd4201 -wd4189 -wd4100 -wd4204 -wd4996 -Z7 %* -Fe:input_duplicator ..\src\main.c /link /manifest:embed
@echo off

popd

:: warning C4189: local variable is initialized but not referenced
:: warning C4100: unreferenced formal parameter
:: warning C4204: nonstandard extension used: non-constant aggregate initializer (C struct designated initializer syntax)
:: warning C4996: 'strdup': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _strdup. See online help for details.
