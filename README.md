# amavisd-milter

amavisd-milter is a milter interface for [amavis](https://www.amavis.org).

## Installing

The simplest way to compile amavisd-milter:
```
curl -L -o amavisd-milter-1.6.1.tar.gz https://github.com/prehor/amavisd-milter/archive/1.6.1.tar.gz
tar xfz amavisd-milter-1.6.1.tar.gz
cd amavisd-milter-1.6.1
sh autoconf.sh.in
./configure
make all
make install
make clean
```

For more information on installation, see [INSTALL](INSTALL).

## Usage

A complete description of usage, configuration and troubleshooting is at
[amavisd-milter(8) manual page](AMAVISD-MILTER.md).

## Reporting Issues

Issues can be reported by using [GitHub Issues](/../../issues). Full details on
how to report issues can be found in the [Contribution Guidelines](CONTRIBUTING.md).

## Contributing

Please read the [Contribution Guidelines](CONTRIBUTING.md), and ensure you are
signing all your commits with
[DCO sign-off](CONTRIBUTING.md#developer-certification-of-origin-dco).

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available,
see the [tags on this repository](/../../tags).

## Authors

* [Petr Řehoř](https://github.com/prehor) - Initial work.

See also the list of
[contributors](https://github.com/prehor/amavisd-milter/contributors)
who participated in this project.

For contributors up to amavisd-milter-1.6.1 see [CHANGELOG](CHANGES).

## License

This project is licensed under the BSD-3-Clause License - see the
[LICENSE](LICENSE) file for details.
