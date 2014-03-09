nec86play
=========

nec86play - PC-9801-86 sound board test program on OpenBSD/luna88k

This is my experimental program to use the PCM part of PC-9801-86 sound board on
OpenBSD/luna88k.

Preparation
-----------
To use this program, you need to apply the following diff.

https://gist.github.com/ao-kenji/9430739

Then add the following device description to your 'config' file.
Or PC98EX config file, which is included in above diff, can be used.
```
# PC-9801 extension slot
pc98ex0         at mainbus0
```
Build the new kernel, then reboot your system.  You may see
```
pc98ex0 at mainbus0
```
in dmesg.  Then, add device files in /dev.
```
% cd /dev
% sudo mknod -m 660 pcexmem c 25 0
% sudo mknod -m 660 pcexio c 25 1
```

Compile
-------

Build by 'make'.  The executable binary is 'nec86play'.

Run
---
```
% ./nec86play
Usage: nec86play [options] wavfile.wav
        -d      : debug flag
        -r #    : sampling rate
        wavfile must be LE, 16bit, stereo

% ./nec86play -r 22050 sample.wav
```
'wav' file must be in little-endian, 16bit, and stereo for now.
