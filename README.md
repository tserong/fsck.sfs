# fsck.sfs

fsck like tool to check for consistency problems on the backing store of the
[s3gw][1] and fix them

## Usage

```shell
Allowed Options:
  -h [ --help ]         print this help text
  -F [ --fix ]          fix any inconsistencies found
  -p [ --path ] arg     path to check
  -q [ --quiet ]        run silently
  -v [ --verbose ]      more verbose output

Must supply path to check.
```

## Checks

<!-- markdownlint-disable line-length-->
| Check              | Fix                                             | Description                                              |
| ------------------ | ----------------------------------------------- | -------------------------------------------------------- |
| metadata integrity | N/A                                             | runs sqlite integrity check on metadata                  |
| metadata version   | N/A                                             | checks metadata schema version is supported by fsck.sfs |
| orphaned objects   | move orphaned objects to "lost+found" directory | locates objects that are not listed in the metadata      |
| orphaned metadata  | unimplemented                                   | locates metadata for which objects don't actually exist  |
| object integrity   | unimplemented                                   | verifies object metadata against file contents on disk   |
<!-- markdownlint-restore -->

## Development

Build the tool with CMake:

```shell
cd fsck.sfs
cmake -S src -B build
cmake --build build
```

Run the tool you just built:

```shell
build/fsck.sfs /path/to/store
```

Or, build and run it in a container using Docker or Podman:

```shell
# Docker
docker build -t fsck.sfs .
docker run -v /path/to/store:/volume fsck.sfs /volume

# Podman
podman build -t fsck.sfs .
podman run -v /path/to/store:/volume fsck.sfs /volume
```

[1]: https://s3gw.io
