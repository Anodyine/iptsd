// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/types.hpp>
#include <core/generic/config.hpp>
#include <core/generic/device.hpp>
#include <core/linux/config-loader.hpp>
#include <core/linux/hidraw-device.hpp>
#include <hid/descriptor.hpp>
#include <ipts/data.hpp>

#include <CLI/CLI.hpp>
#include <cstdlib>
#include <gsl/gsl>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

namespace iptsd::apps::check {

static int run(const gsl::span<char *> args)
{
	CLI::App app {"Utility for checking if a hidraw device is an IPTS touchscreen."};

	std::filesystem::path path {};
	app.add_option("DEVICE", path)
		->description("The hidraw device node of the device.")
		->type_name("FILE")
		->required();

	bool quiet = false;
	app.add_flag("-q,--quiet", quiet)->description("Disable output of device information.");

	CLI11_PARSE(app, args.size(), args.data());

	if (quiet)
		spdlog::set_level(spdlog::level::off);

	// Open the device
	const core::linux::HidrawDevice device {path};

	const core::DeviceInfo info = device.info();
	const std::optional<const ipts::Metadata> metadata = device.get_metadata();

	spdlog::info("Opened device {:04X}:{:04X}", info.vendor, info.product);

	// Check if the device can switch modes
	if (!device.has_set_mode()) {
		spdlog::error("{} is not an IPTS device!", path.string());
		return EXIT_FAILURE;
	}

	// Check if the device can send touch data.
	if (!device.has_touch_data()) {
		spdlog::error("{} is not an IPTS device!", path.string());
		return EXIT_FAILURE;
	}

	// Check if the device has a config
	const core::linux::ConfigLoader loader {info, metadata};
	const core::Config config = loader.config();

	if (config.width == 0 || config.height == 0) {
		spdlog::error("{} is not an IPTS device!", path.string());
		return EXIT_FAILURE;
	}

	spdlog::info("{} is an IPTS device!", path.string());

	return 0;
}

} // namespace iptsd::apps::check

int main(int argc, char **argv)
{
	spdlog::set_pattern("[%X.%e] [%^%l%$] %v");
	const gsl::span<char *> args {argv, gsl::narrow<usize>(argc)};

	try {
		return iptsd::apps::check::run(args);
	} catch (std::exception &e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}
}