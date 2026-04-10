#pragma once

// #include <tl/expected.hpp>

enum class ReaderResult {
    Success = 0,
    CorruptedFrame,
    OutOfBound,
    FrameSearchTimeout,
    FileError,
    UnkownError,
    PostProcessorError,
    InternalError,
    InitError,
};
