#include <slim/runtime.h>

slim::common::io::Runtime& slim::runtime::instance() {
    static slim::common::io::Runtime runtime{4};
    return runtime;
}
