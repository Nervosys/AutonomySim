<#
FILENAME:
  test_unrealasset.psm1
DESCRIPTION:
  PowerShell script to validate Unreal Engine SUV asset version.
AUTHOR:
  Adam Erickson (Nervosys)
DATE:
  11-17-2023
NOTES:
  Assumes: PowerShell version >= 7 and Visual Studio 2022 (version 17).
  Script is intended to run from AutonomySim base project directory.

  Copyright © 2023 Nervosys, LLC
#>

###
### Variables
###

[string]$ADVANCED_VEHICLE_DIR = 'Unreal\Plugins\AutonomySim\Content\VehicleAdv'  # advanced vehicle template directory
[string]$ASSET_SUV_VERSION = '1.2.0'
[string]$ASSET_SUV_URL = "https://github.com/Microsoft/AirSim/releases/download/v$ASSET_SUV_VERSION/car_assets.zip"

###
### Functions
###

function Remove-Directories {
  param(
      [Parameter()]
      [String[]]
      $Directories = @('temp', 'external')
  )
  foreach ($d in $Directories) {
      Remove-Item -Path "$d" -Force -Recurse
  }
}

function Invoke-Fail {
  param(
      [Parameter()]
      [String]
      $ProjectDir = "$PWD",
      [Parameter()]
      [Switch]
      $RemoveDirs = $false
  )
  Set-Location $ProjectDir
  if ($RemoveDirs) -eq $true { Remove-Directories }
  Write-Error 'Error: Build failed. Exiting Program.' -ErrorAction Stop
}

function Test-AssetSuvVersion {
  param(
    [Parameter(Mandatory)]
    [Boolean]
    $FullPolySuv = $false
  )
  if ($FullPolySuv -eq $true) {
    if ( -not (Test-Path -LiteralPath "${ADVANCED_VEHICLE_DIR}\SUV\v${ASSET_SUV_VERSION}") ) {
      # Create advanced vehicle template directory if it does not exist
      [System.IO.Directory]::CreateDirectory("$ADVANCED_VEHICLE_DIR")
            
      Write-Output ''
      Write-Output '-----------------------------------------------------------------------------------------'
      Write-Output ' Downloading 37 MB high-polycount SUV asset.'
      Write-Output ' To skip asset installation, run this script without the `-FullPolySuv` flag'
      Write-Output '-----------------------------------------------------------------------------------------'
      Write-Output ''

      # Remove temp SUV directory if it already exists
      Remove-Item 'temp\suv_download' -Force -Recurse -ErrorAction Ignore
      [System.IO.Directory]::CreateDirectory('temp\suv_download')

      # Download SUV asset
      [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
      Invoke-WebRequest $ASSET_SUV_URL -OutFile 'temp\suv_download\car_assets.zip'

      Remove-Item "${ADVANCED_VEHICLE_DIR}\SUV" -Force -Recurse
      Expand-Archive -Path 'temp\suv_download\car_assets.zip' -DestinationPath $ADVANCED_VEHICLE_DIR
      Remove-Item 'temp\suv_download' -Force -Recurse

      # If high-polycount SUV is unable to download, notify users that gokart will be used
      if ( -not (Test-Path -LiteralPath "${ADVANCED_VEHICLE_DIR}\SUV") ) {
        Write-Output 'Download of high-polycount SUV failed. AutonomySim will use the default vehicle.'
        Invoke-Fail
      }
    }
    else {
      Write-Output "High-poly SUV asset version found: ${ASSET_SUV_VERSION}."
    }
  }
  else {
    Write-Output 'Download of high-poly SUV asset disabled. Default vehicle will be used.'
  }
}

###
### Exports
###

Export-ModuleMember -Variable ASSET_SUV_VERSION
Export-ModuleMember -Function Test-AssetSuvVersion
