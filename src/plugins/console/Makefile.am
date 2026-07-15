module_LTLIBRARIES = console.la
console_la_SOURCES = console.cpp
#console_la_LDFLAGS = -module -version-info $(MODULE_VERSION_INFO)
console_la_LDFLAGS = -module -version-info 1:2:3

print-vars:
	@echo "MODULE_VERSION_INFO=$(MODULE_VERSION_INFO)"
	@echo "console_la_LDFLAGS=$(console_la_LDFLAGS)"
	@echo "module_LTLIBRARIES=$(module_LTLIBRARIES)"