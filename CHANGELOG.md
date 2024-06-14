# Changelog

## [0.24] 2024-06-14
### Added
- File transfer action
- Atari 2600 cartridge reader adapter 3d printed connector
- Address mask in device configuration settings
- Added 2364/2332 mask ROM device configurations
- Mask ROM adapter pcb

### Changed
- Global file selection routines
- Default to fetch Pico SDK from git

### Fixed
- Added post-delay to XMODEM transfers
- Error checking during image write process

## [0.23] 2024-04-24
### Added
- Atari 2600 cartridge reader adapter pcb
- Tool menu with specialized write and verification commands
- Local flash filesystem to store and retrieve ROM images (littlefs)

### Changed
- XMODEM communication moved into submodule
- Menu & device systems rewritten and project reorganized
- Assume full data reading/writing hardware capabilities

## [0.22] 2024-03-27
### Added
- Hardware PCB design
- 3D printed case
- ROM read/verification
- XMODEM send
- Page read

### Changed
- Menu items for image and page read

### Fixed
- Disable automatic carriage return
- Prompt typos
- Ignore backspaces and NAKs when inappropriate
- Increased EOT timeout
- Improved verbosity
