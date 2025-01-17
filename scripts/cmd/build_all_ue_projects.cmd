REM build_all_ue_projects.cmd
REM
REM Updated for Windows 10/11
REM

@echo on

REM //---------- set up variable ----------
setlocal

set ROOT_DIR=%cd%

REM should rebuild?
set RebuildAutonomySim=%1
REM root folder containing all Unreal projects you want to build
set UnrealProjsPath=%2
REM Output folder where the builds would be stored
set OutputPath=%3
REM folder containing AutonomySim repo
set AutonomySimPath=%4
REM path for UE toolset
set ToolPath=%5

REM set defaults if ars are not supplied
if not defined UnrealProjsPath ( set "UnrealProjsPath=D:\vso\msresearch\Theseus" )
if not defined AutonomySimPath ( set "AutonomySimPath=C:\GitHubSrc\AutonomySim" )
if not defined OutputPath ( set "OutputPath=%ROOT_DIR%build" )
if not defined RebuildAutonomySim ( set "RebuildAutonomySim=true" )

REM re-build AutonomySim
if %RebuildAutonomySim%==true (
  cd /D %AutonomySimPath%
  call clean
  call build
  if %ERRORLEVEL% == 1 goto :failed
  cd /D %ROOT_DIR%
)

if not exist %OutputPath% ( mkdir %OutputPath% )

call:doOneProject "TalkingHeads"
call:doOneProject "ZhangJiaJie"
call:doOneProject "AutonomySimEnvNH"
call:doOneProject "SimpleMaze"
call:doOneProject "LandscapeMountains"
call:doOneProject "Africa_001" "Africa"
call:doOneProject "Forest"
call:doOneProject "Coastline"
call:doOneProject "TrapCamera"
call:doOneProject "CityEnviron"
call:doOneProject "Warehouse"
call:doOneProject "Plains" "" 4.19

goto :done

:doOneProject
REM args: OutputPath ToolPath UEVer
if not defined 2 (
  cd /D %UnrealProjsPath%\%~1
) else (
  cd /D %UnrealProjsPath%\%~2
)
if %ERRORLEVEL%==1 ( goto :failed )

robocopy %AutonomySimPath%\Unreal\Environments\Blocks . *.cmd  /njh /njs /ndl /np

call update_from_git.cmd %AutonomySimPath%
if %ERRORLEVEL%==1 ( goto :failed )

call package.cmd %OutputPath% %ToolPath% %~3
if %ERRORLEVEL%==1 ( goto :failed )

goto :done

:failed
echo "Error occured while building all UE projects"
exit /b 1

:done
cd %ROOT_DIR%
if not defined 1 ( pause )
