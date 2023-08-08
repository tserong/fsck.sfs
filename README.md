# fsck.s3gw

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
| Check            | Fix                                             | Description                                         |
| ---------------- | ----------------------------------------------- | --------------------------------------------------- |
| orphaned objects | move orphaned objects to "lost+found" directory | locates objects that are not listed in the metadata |
<!-- markdownlint-restore -->

## Development

Build the tool with CMake:

```shell
cd fsck.s3gw
cmake -S src -B build
cmake --build build
```

Run the tool you just built:

```shell
build/fsck.s3gw /path/to/s3gw/store
```

Or, build and run it in a container using Docker or Podman:

```shell
# Docker
docker build -t fsck.s3gw .
docker run -v /path/to/s3gw/store:/volume fsck.s3gw /volume

# Podman
podman build -t fsck.s3gw .
podman run -v /path/to/s3gw/store:/volume fsck.s3gw /volume
```

[1]: https://s3gw.io
