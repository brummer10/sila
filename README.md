# sila

A little Gtkmm based LADSPA Browser and Host

![Puplic Domain](http://freedomdefined.org/upload/2/20/Pd-button.png)

Select LADSPA plugins from a list and run them as jack standalone apps.
Or give the lib to load and optional the ID to run a plugin direct.

example:

- sila amp.so 1048

![sila](https://github.com/brummer10/sila/raw/master/sila.png)

    
Depends:

- gtkmm3
- jack


Install:

- make
- (sudo) make install
- see makefile for more options

Debian:

To build a debian package just run:

- make deb
