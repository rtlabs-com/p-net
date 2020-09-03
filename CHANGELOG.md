# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## [Unreleased]


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
