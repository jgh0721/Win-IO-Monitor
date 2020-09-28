# Windows IO Monitor 

## Goals

    1. File I/O Monitor and Control 
    2. Registry I/O Monitor and Control
    3. Process Creation and Termination Monitor and Control

## Support OS

    Windows XP SP2 and Later.

## Build Environments

    Driver 

        Windows Driver Kit 7.1 ( 7600.16385.1 )
        Visual Studio 2015

    API 
        Visual Studio 2017 

    Control 
        Visual Studio 2017 
        Qt 5.14.2 
        QtitanDataGrid, but because this module is commercial sw, it's source code were note included. 

# Windows Isolation Minifilter Driver

## Golas 

  1. Implements Shadow File Object functionality
  2. Implements Global & Process Filter 
  3. Implements Encryption 

## Support OS

    Windows XP SP2 and Later.
    X86 and X64 

## Build Environments

### Driver

    1. Windows Driver Kits 7.1 ( 7600.16385.1 )
        * Install To C:\WinDDk 
    2. Visual Studio 2015 or Later

## Test

    1. VM Prepare( Windows XP SP2 or Later, SP3 Recommended )
    2. register scripts\RegisterIsolationDriver.reg to make driver service 
    3. copy WinIOIsolation.sys to VM's %WINDIR%\system32\drivers 
    4. open *cmd* and *fltmc load WinIOIsolation*

    * Currently, this filter only apply to file filename contains 'isolationtest.txt' 

# External Open Source Modules

    * https://github.com/xiao70/X70FSD
    * https://github.com/ned14/ntkernel-error-category
