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

[1]: https://s3gw.io
