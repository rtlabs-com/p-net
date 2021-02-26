# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## [Unreleased]

## 2021-03-03
### Changed
- Set thread priority and stack size via configuration

## 2021-02-19
### Changed
- Split options.h into two files.
- Simplify configuration

## 2020-02-03
### Added
 - Improved support for multiple ports

## 2021-01-29
### Changed
 - Move remaining Linux sample app files to src/ports/linux

## 2020-01-11
### Added
 - Support for multiple ports

## 2021-01-05
### Added
 - Additional API functions for low-level diagnostic handling

## 2020-12-22
### Changed
 - Remove link status from configuration

## 2020-12-15
### Changed
 - Use Profinet version 2.4 for the GSDML file
 - Changed default installation path for sample_app on Raspberry Pi

## 2020-12-11
### Added
 - Runtime data for multiple ports
 - Functionality to search for scripts in multiple locations for Linux

## 2020-12-07
### Changed
 - Rename pnet_set_state() to pnet_set_primary_state()

## 2020-12-01
### Added
- Changed informative callbacks to be void-functions.

## 2020-11-30
### Added
- SNMP functionality for Linux

## 2020-11-20
### Added
- Additional log level FATAL

## 2020-11-19
### Added
- SNMP functionality for rt-kernel

## 2020-11-16
### Added
- Support for multiple ports in configuration

## 2020-11-10
### Changed
- Remove LLDP TTL from configuration
- Remove LLDP chassis ID from configuration

## 2020-11-03
### Changed
- Improve API for adding diagnosis
- Use rtlabs-com/osal and rtlabs-com/cmake-tools as submodules

## 2020-10-28
### Changed
- Rename compile time constants for number of slots and subslots

## 2020-09-19
### Changed
- Modified public API for sending alarm ACK.

## 2020-08-24
### Changed
- Renamed configuration members related to I&M data

## 2020-08-18
### Added
- Add min_device_interval to the configuration
- Use cmake to set configurable build options

## 2020-06-15
### Added
- New user callback for LED state change.

## 2020-06-11
### Added
- New API function for application to perform a factory reset

## 2020-04-09
### Added
- New user callback to indicate reset requests from the IO-controller.

## 2020-04-06
### Changed
- Changed the user API to use a handle to the p-net stack, and to allow user
  arguments to callbacks.

## 2020-03-27
### Changed
- Read MAC address also from Linux hardware.
