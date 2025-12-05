#include "config.h"
#include <iostream>
#include <slim/slim.h>
#include <slim/common/exception.h>
#include <slim/common/log.h>
int main(int argc, char *argv[]) {
    using namespace slim::common;
    try {
        log::trace(log::Message("main()", "begins",__FILE__, __LINE__));
        log::debug(log::Message("main()", "top of try",__FILE__, __LINE__));
        slim::start(argc, argv);
        log::debug(log::Message("main()", "bottom of try",__FILE__, __LINE__));
    }
    catch(std::string error) {
        log::debug(log::Message("main()", "caught exception",__FILE__, __LINE__));
        log::error(log::Message("main()", error,__FILE__, __LINE__));
        slim::stop();
        log::trace(log::Message("main()", "exiting early",__FILE__, __LINE__));
        exit(1);
    }
    catch(const slim::common::SlimFileException& error) {
        std::string error_message = error.message + ", path => " + error.path;
        log::error(log::Message(error.call, error_message,__FILE__, __LINE__));
        slim::stop();
        log::trace(log::Message("main()", "exiting early",__FILE__, __LINE__));
        exit(1);
    }
    catch(const slim::common::SlimException& error) {
        log::error(log::Message(error.call, error.message,__FILE__, __LINE__));
        slim::stop();
        log::trace(log::Message("main()","exiting early",__FILE__, __LINE__));
        exit(1);
    }
    catch (const std::invalid_argument& error) {
        log::error(log::Message("main()", error.what(),__FILE__, __LINE__));
        slim::stop();
        log::trace(log::Message("main()","exiting early",__FILE__, __LINE__));
        exit(1);
    }
    catch (const std::runtime_error& error) {
        log::error(log::Message("main()", error.what(),__FILE__, __LINE__));
        slim::stop();
        log::trace(log::Message("main()","exiting early",__FILE__, __LINE__));
        exit(1);
    }
    catch (const std::exception& error) {
        log::error(log::Message("main()", error.what(),__FILE__, __LINE__));
        slim::stop();
        log::trace(log::Message("main()","exiting early",__FILE__, __LINE__));
        exit(1);
    }
    catch(...) {
        log::error(log::Message("main()", "caught unknown exception",__FILE__, __LINE__));
        slim::stop();
        log::trace(log::Message("main()","exiting early",__FILE__, __LINE__));
        exit(1);
    }
    slim::stop();
    log::trace(log::Message("main()","exiting normally",__FILE__, __LINE__));
    return 0;
}