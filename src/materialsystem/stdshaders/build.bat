@echo off
setlocal

rem ================================
rem ==== MOD PATH CONFIGURATIONS ===

rem == Set the absolute path to your mod's game directory here ==
set GAMEDIR=E:\cs_source\mp\game\csso

rem == Set the relative or absolute path to Source SDK Base 2013 Singleplayer\bin ==
set SDKBINDIR=E:\Games\Steam\steamapps\common\Counter-Strike Source\bin

rem ==  Set the Path to your mod's root source code ==
rem This should already be correct, accepts relative paths only!
set SOURCEDIR=..\..

rem ==== MOD PATH CONFIGURATIONS END ===
rem ====================================


call buildcssoshaders.bat