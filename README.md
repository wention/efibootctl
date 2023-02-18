efibootctl
==========

set efi bootnext for windows

# Usage

## list boot info

```
$ efibootctl

BootNext: 0004
BootCurrent: 0000
BootOrder: 0005, 0004, 0000, 0001, 2001, 2002, 2003
Boot0000: Windows Boot Manager
Boot0001: EFI Hard Drive (50026B7685E14427-KINGSTON SKC3000D2048G)
Boot0002: EFI PXE 0 for IPv4 (98-8F-E0-64-31-7E) 
Boot0003: EFI PXE 0 for IPv6 (98-8F-E0-64-31-7E) 
Boot0004: Manjaro
Boot0005: Windows Boot Manager
```

## set boot next

```
efibootctl -n 4
```