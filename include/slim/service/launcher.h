#ifndef __SLIM__SERVICE__LAUNCHER__H
#define __SLIM__SERVICE__LAUNCHER__H
#include <v8.h>
#include <string>
#include <vector>
#include <slim/module/import_specifier.h>
namespace slim::service::launcher {
	void marshal_resources();
	void launch(slim::module::variant_specifier _script_name_string_or_specifier_stub);
}
#endif