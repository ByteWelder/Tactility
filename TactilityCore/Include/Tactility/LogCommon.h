#pragma once

namespace tt {

/** Used for log output filtering */
enum class LogLevel : int {
    Error, /*!< Critical errors, software module can not recover on its own */
    Warning, /*!< Error conditions from which recovery measures have been taken */
    Info, /*!< Information messages which describe normal flow of events */
    Debug, /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    Verbose /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
};

}