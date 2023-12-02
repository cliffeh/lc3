# popt-redist
a redistributable version of the popt argument parsing library 

## Using
Place the contents of this repo into a subdirectory of your project. (For the purposes of this doc, we'll assume this subdirectory is named `popt`.) If you are using autotools for your main project, you'll need to make the following modifications:

* Add `AC_CONFIG_SUBDIRS([popt])` to your main project's configure.ac
* Add `SUBDIRS = popt` to your main project's Makefile.am

This should be sufficient - i.e., in your main project you should be able to do the standard:
```
./configure
make
```

...and then add `popt/libpopt.a` as a library dependency to whichever targets you like.

If you are *not* using autotools for your main project, `autogen.sh` is provided as a convenient way to generate a configure script and a Makefile that you can use. For example, within the `popt` subdirectory:
```
./autogen.sh
./configure
make
```

This will build `libpopt.a` as above, which you can then link however you see fit.

## Motivations
[popt](https://github.com/rpm-software-management/popt) is an excellent argument parsing library - full-featured, battle-hardened (it's been around for decades now), etc. It is also widely-available at this point..._but_:

* It is not included by default on all distributions/platforms, and
* Even when libpopt is included by default, the development headers (required for building software from source) often are _not_

This repo is an attempt to repackage popt such that it can be "parachuted" into a development project with minimal refactoring/hassle. It's not so much making it "redistributable" - popt's [license](COPYING) already allows for that - it's more about making it readily re-packageable.
