// Default headers
#include <tactility/device.h>
#include <tactility/dts.h>
#include <tactility/module.h>
// DTS headers
#include <test_include.h>

static const root_config_dt root_config = {
	"Test Model"
};

static struct Device root = {
	.address = 0,
	.name = "/",
	.config = &root_config,
	.flags = DEVICE_FLAG_DTS,
	.parent = NULL,
	.internal = NULL
};

static const generic_device_config_dt test_device_config = {
	0,
	42,
	"hello"
};

static struct Device test_device = {
	.address = 0,
	.name = "test-device",
	.config = &test_device_config,
	.flags = DEVICE_FLAG_DTS,
	.parent = &root,
	.internal = NULL
};

static const bool_device_config_dt bool_test_device_config = {
	true,
	false,
	true,
	true,
	true
};

static struct Device bool_test_device = {
	.address = 0,
	.name = "bool-test-device",
	.config = &bool_test_device_config,
	.flags = DEVICE_FLAG_DTS,
	.parent = &root,
	.internal = NULL
};

const struct DtsDevice dts_devices[] = {
	{ &root, "test,root", DTS_DEVICE_STATUS_OKAY },
	{ &test_device, "test,generic-device", DTS_DEVICE_STATUS_OKAY },
	{ &bool_test_device, "test,bool-device", DTS_DEVICE_STATUS_OKAY },
	DTS_DEVICE_TERMINATOR
};

extern struct Module data_module;

struct Module* const dts_modules[] = {
	&data_module,
	NULL
};
