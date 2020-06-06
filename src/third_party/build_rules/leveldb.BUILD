package(default_visibility = ["//visibility:public"])

cc_library(
    name = "leveldb",
    srcs = glob(
        ["**/*.cc"],
        exclude = [
            "doc/**",
            "**/*_test.cc",
            "util/env_windows.cc",
        ],
    ),
    hdrs = glob(
        ["**/*.h"],
        exclude = [
          "doc/**",
          "util/windows_logger.h",
          "util/env_windows*.h",
        ],
    ),
    includes = ["include"],
    defines = [
        "LEVELDB_IS_BIG_ENDIAN=0",
        "LEVELDB_PLATFORM_POSIX=1",
    ],
    linkstatic = 1,
)
