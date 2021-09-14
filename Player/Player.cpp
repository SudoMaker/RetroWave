/*
    This file is part of RetroWave.

    Copyright (C) 2021 ReimuNotMoe <reimu@sudomaker.com>
    Copyright (C) 2021 Yukino Song <yukino@sudomaker.com>


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Player.hpp"

RetroWavePlayer player;

std::tuple<size_t, size_t, size_t> RetroWavePlayer::sec2hms(size_t _secs) {
	size_t mins = _secs / 60;
	size_t secs = _secs % 60;
	size_t hrs = mins / 60;
	mins = mins % 60;

	return {hrs, mins, secs};
}

static std::vector<std::string> string_split(const std::string &s, char delim) {
	std::vector<std::string> result;
	std::stringstream ss (s);
	std::string item;

	while (getline(ss, item, delim)) {
		result.push_back(item);
	}

	return result;
}

void RetroWavePlayer::set_nonblocking(int fd_, bool __nonblocking) {
	int flags = fcntl(fd_, F_GETFL, 0);
	if (flags == -1)
		throw std::system_error(errno, std::system_category(), "fcntl F_GETFL");

	flags = __nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);

	if (fcntl(fd_, F_SETFL, flags))
		throw std::system_error(errno, std::system_category(), "fcntl F_SETFL");
}

void RetroWavePlayer::init() {
	init_retrowave();
	init_tinyvgm();
	init_term();
}

void RetroWavePlayer::init_term() {
	term_attr_save();
	term_attr_disable_buffering();
}

void RetroWavePlayer::init_retrowave() {
	retrowave_io_init(&rtctx);
	reset_chips();
	usleep(200 * 1000);
}

void RetroWavePlayer::init_tinyvgm() {
	tinyvgm_init(&tvc);

	std::unordered_map<uint8_t, int (*)(void *, uint8_t, const void *, uint32_t)> cmd_cb_map = {
		{0x5a, callback_opl2},
		{0x5e, callback_opl3_port0},
		{0x5f, callback_opl3_port1},
		{0xbd, callback_saa1099},
		{0x50, callback_sn76489_port0},
		{0x51, callback_ym2413},
		{0x30, callback_sn76489_port1},
	};

	for (auto &it : disabled_vgm_commands) {
		cmd_cb_map.erase(it);
		printf("info: disabled VGM command %02x\n", it);
	}

	for (auto &it : cmd_cb_map) {
		tinyvgm_add_command_callback(&tvc, it.first, it.second, this);
		printf("debug: VGM cmd %02x handler %p\n", it.first, it.second);
	}

//	tinyvgm_add_command_callback(&tvc, 0x5a, callback_opl2, this);
//	tinyvgm_add_command_callback(&tvc, 0x5e, callback_opl3_port0, this);
//	tinyvgm_add_command_callback(&tvc, 0x5f, callback_opl3_port1, this);
//
//	tinyvgm_add_command_callback(&tvc, 0xbd, callback_saa1099, this);
//	tinyvgm_add_command_callback(&tvc, 0x50, callback_sn76489_port0, this);
//	tinyvgm_add_command_callback(&tvc, 0x51, callback_ym2413, this);
//	tinyvgm_add_command_callback(&tvc, 0x30, callback_sn76489_port1, this);


	tinyvgm_add_command_callback(&tvc, 0x61, callback_sleep, this);
	tinyvgm_add_command_callback(&tvc, 0x62, callback_sleep_62, this);
	tinyvgm_add_command_callback(&tvc, 0x63, callback_sleep_63, this);

	for (int i = 0x70; i <= 0x7f; i++) {
		tinyvgm_add_command_callback(&tvc, i, callback_sleep_7n, this);
	}

	tinyvgm_add_header_callback(&tvc, 0x18, callback_header_total_samples, this);

	tinyvgm_add_event_callback(&tvc, TinyVGM_Event_HeaderParseDone, callback_header_done, this);
	tinyvgm_add_event_callback(&tvc, TinyVGM_Event_PlaybackDone, callback_playback_done, this);
}

void RetroWavePlayer::parse_disabled_vgm_commands(const std::string &str) {
	if (str.empty()) {
		return;
	}

	auto strs = string_split(str, ',');

	for (auto &it : strs) {
		uint8_t cmd = strtoul(it.c_str(), nullptr, 16);
		disabled_vgm_commands.insert(cmd);
	}

	for (auto &it : disabled_vgm_commands) {
		printf("info: disabled VGM command 0x%02x\n", it);
	}

	usleep(500 * 1000);
}

bool RetroWavePlayer::load_file(const std::string &path) {
	int fd = open(path.c_str(), O_RDONLY);

	if (fd < 0) {
		printf("error: failed to open file `%s': %s\n", path.c_str(), strerror(errno));
		return false;
	}

	const size_t io_size = 4096;

	size_t read_len = 0;

	while (1) {
		file_buf.resize(read_len + io_size);

		ssize_t rc = read(fd, file_buf.data() + read_len, io_size);

		if (rc > 0) {
			read_len += rc;
		} else if (rc == 0) {
			file_buf.resize(read_len);
			break;
		} else {
			printf("error: failed to read file `%s': %s\n", path.c_str(), strerror(errno));
			close(fd);
			return false;
		}
	}

	static const uint8_t vgm_header[] = "Vgm ";

	bool ret = false;
	bool gz_tried = false;
	z_stream zlib_strm = {0};
	size_t zlib_decompressed_size = 0;
	std::vector<uint8_t> zlib_decompress_buf;

retry_verify_file:
	if (file_buf.size() > 32) {
		if (memcmp(file_buf.data(), vgm_header, 4) != 0) { // VGM header not found, try gunzip
			if (gz_tried) {
				printf("info: 2nd try failed\n");
				goto done;
			}

			printf("info: VGM header not found, try gunzip\n");

			inflateInit2(&zlib_strm, 15|32);
			gz_tried = true;

			zlib_strm.next_in = file_buf.data();
			zlib_strm.avail_in = file_buf.size();

			while (1) {
				zlib_decompress_buf.resize(zlib_decompressed_size + io_size);

				zlib_strm.next_out = zlib_decompress_buf.data() + zlib_decompressed_size;
				zlib_strm.avail_out = io_size;

				int zlib_rc = inflate(&zlib_strm, Z_NO_FLUSH);

				if (zlib_rc >= 0) {
					auto out_size = io_size - zlib_strm.avail_out;
//					printf("info: zlib decompressed %lu bytes\n", out_size);
					zlib_decompressed_size += out_size;

					if (zlib_rc == Z_STREAM_END) {
						inflateEnd(&zlib_strm);
//						printf("info: zlib decompress done, total %zu bytes\n", zlib_decompressed_size);
						zlib_decompress_buf.resize(zlib_decompressed_size);
						std::swap(file_buf, zlib_decompress_buf);
						goto retry_verify_file;
					}
				} else {
					inflateEnd(&zlib_strm);
					printf("zlib error %d in file `%s'!\n", zlib_rc, path.c_str());
					break;
				}
			}
		} else { // Header found
			ret = true;
		}
	} else {
		printf("error: file too small!\n");
	}

done:
	close(fd);
	return ret;
}

void RetroWavePlayer::play(const std::vector<std::string> &file_list) {
	for (size_t i=0; i < file_list.size(); ) {
		auto &cur_file = file_list[i];

		current_file = cur_file.c_str();
		current_track = i+1;
		total_tracks = file_list.size();

		term_clear();

		reg_map.clear();
		reg_map_refreshed_list.clear();
		regmap_sn76489.used = false;

		printf("Now playing (%zu/%zu): %s\n", current_track, total_tracks, current_file);

		if (!load_file(cur_file)) {
			i++;
			continue;
		}

		int32_t rc_tv_parse, rc_gd3_parse;

		rc_tv_parse = tinyvgm_parse(&tvc, file_buf.data(), 32);	// GD3 offset must be within first 32 bytes

		if (rc_tv_parse == 32) {
			auto gd3_offset = tvc.header_info.gd3_offset;
			auto gd3_size = file_buf.size() - gd3_offset;

			if (gd3_offset) {
				tinyvgm_init_gd3(&gd3_info);
				rc_gd3_parse = tinyvgm_parse_gd3(&gd3_info, file_buf.data()+gd3_offset, gd3_size);
				if (rc_gd3_parse) {
					gd3_to_info(&gd3_info);
				}
			}
		} else {
			printf("TinyVGM error: failed to process file `%s', rc=%d\n", cur_file.c_str(), rc_tv_parse);
			tinyvgm_reset(&tvc);
			i++;
			continue;
		}

		const size_t parse_size_hint = 32;

		for (size_t j=32; j<file_buf.size(); j+=parse_size_hint) {
			size_t parse_size = parse_size_hint;

			if (parse_size_hint + j > file_buf.size()) {
				parse_size = file_buf.size() - j;
			}

			rc_tv_parse = tinyvgm_parse(&tvc, file_buf.data()+j, parse_size);

			if (rc_tv_parse == INT32_MIN) {
				printf("TinyVGM error: failed to process file `%s', rc=%d\n", cur_file.c_str(), rc_tv_parse);
				break;
			}

			played_bytes += rc_tv_parse;

			if (playback_done) {
				break;
			}

			if (key_command & 0x0c) {
				break;
			}
		}

		if (key_command == PREV) {
			if (i)
				i--;
		} else {
			i++;
		}

		key_command = NONE;

		playback_reset();

		usleep(50 * 1000);
	}
}

void RetroWavePlayer::playback_reset() {
	playback_done = false;
	played_samples = 0;
	last_slept_samples = 0;
	last_last_slept_samples = 0;
	total_samples = 0;
	last_slept_usecs = 0;
	played_bytes = 0;
	last_secs = 0;

	metadata = Metadata();
	tinyvgm_destroy_gd3(&gd3_info);
	tinyvgm_reset(&tvc);
	reset_chips();
	usleep(200 * 1000);
}

void RetroWavePlayer::do_exit(int rc) {
	reset_chips();
	term_attr_load();
	exit(rc);
}

void int_handler(int signal) {
	player.do_exit(1);
	exit(1);
}

void ShowHelpExtra() {
	puts("");

	puts(" Keyboard commands:\n"
	     "     q: Quit\n"
	     "     s: Single frame\n"
	     "     Space( ): Pause/Resume\n"
	     "     Slash(/): Hold to Fast forward\n"
	     "     Comma(,): Previous\n"
	     "     Dot(.): Next\n");
#if defined (__CYGWIN__)
	puts("");
	puts(" Windows specific notes:\n"
	     "  1. Windows doesn't have a monotonic self-increasing clock/timer that is unaffected by real world time changes.\n"
	     "     This may make the playback unstable. And the playback will be destroyed if a NTP time update happens in background.\n"
	     "  2. OSD refresh rate is set to 1 second and regmap display is disabled by default because of the laggy conhost.exe terminal.\n"
	     "     If you want to see the register map visualization properly, try using MinTTY as your terminal. Or use another OS.\n"
	     "  3. To specify serial port, write COMx in device path. e.g. -d COM1");
#endif
}

int main(int argc, char **argv) {
	signal(SIGINT, int_handler);

	cxxopts::Options options("Retrowave_Player", "Retrowave_Player - Player for the Retrowave series.");

	std::string device_type, device_path, spi_cs_gpio, test_type, disabled_vgm_cmds;
	std::vector<std::string> positional_args;

#if defined (__CYGWIN__)
	const size_t osd_default_refresh_interval = 1000000000;
	const int osd_default_show_reg = 0;
#elif (defined (__APPLE__) && defined (__MACH__))
	const size_t osd_default_refresh_interval = 1000 * 1000 * 100;
	const int osd_default_show_reg = 1;
#else
	const size_t osd_default_refresh_interval = 1000 * 1000 * 10;
	const int osd_default_show_reg = 1;
#endif

	options.add_options("Main")
		("h,help", "Show this help")

		("t", "Device type (spi/tty)", cxxopts::value<std::string>(device_type)->default_value("tty"))
		("d", "Device path", cxxopts::value<std::string>(device_path)->default_value("/dev/ttyACM0"))
#ifdef __linux__
		("g", "GPIO chip,pin for SPI chip select", cxxopts::value<std::string>(spi_cs_gpio)->default_value("0,6"))
#endif
		("D", "Comma separated list of disabled processing of certain VGM commands in hex", cxxopts::value<std::string>(disabled_vgm_cmds)->default_value(""))
		("i", "OSD refresh interval in ns, 0 to disable", cxxopts::value<size_t>(player.osd_ratelimit_thresh)->default_value(std::to_string(osd_default_refresh_interval)))
		("m", "Show metadata in OSD (1/0)", cxxopts::value<int>(player.osd_show_meta)->default_value(std::to_string(1)))
		("r", "Show chip regs in OSD (1/0)", cxxopts::value<int>(player.osd_show_regs)->default_value(std::to_string(osd_default_show_reg)))
		("T", "Test to run (\"help\" for a list)",  cxxopts::value<std::string>(test_type)->default_value(""))

		;

	options.add_options("positional")
		("positional", "Positional parameters: The files to play", cxxopts::value<std::vector<std::string>>(positional_args))
		;

	options.parse_positional("positional");
	options.positional_help("[FILES...]").show_positional_help();


	try {
		auto cmd = options.parse(argc, argv);

		if (cmd.count("help") || (test_type.empty() && positional_args.empty())) {
			std::cout << options.help({"Main"});
			ShowHelpExtra();
			return 0;
		}

	} catch (std::exception &e) {
		std::cout << "Error: " << e.what() << "\n";
		std::cout << options.help({"Main"});
		ShowHelpExtra();
		return 1;
	}

	if (device_type == "spi") {
#ifdef __linux__
		auto scgs = string_split(spi_cs_gpio, ',');
		int scg[2] = {0};

		if (scgs.size() != 2) {
			puts("error: bad GPIO specification. Please use the `gpiochip,line' format.");
			exit(2);
		}

		scg[0] = strtol(scgs[0].c_str(), nullptr, 10);
		scg[1] = strtol(scgs[1].c_str(), nullptr, 10);

		printf("SPI CS: chip=%d, line=%d\n", scg[0], scg[1]);

		if (retrowave_init_linux_spi(&player.rtctx, device_path.c_str(), scg[0], scg[1])) {
			exit(2);
		}
#else
		puts("error: SPI is not supported on your platform.");
		exit(2);
#endif
	} else if (device_type == "tty") {
#if defined (__CYGWIN__)
		if (device_path.find("COM") == 0) {
			char *comnum_str = const_cast<char *>(device_path.c_str() + 3);
			long comnum = strtol(comnum_str, nullptr, 10);

			device_path = "/dev/ttyS";
			device_path += std::to_string(comnum - 1);
		}
#endif

		if (retrowave_init_posix_serialport(&player.rtctx, device_path.c_str())) {
			exit(2);
		}
	} else {
		puts("Unsupported device type. Please read the help.");
		exit(2);
	}

	int prio = -5;

	// Windows sucks, again
#if defined (__CYGWIN__)
	prio = -20;
#endif

	int rc = setpriority(PRIO_PROCESS, 0, prio);

	if (rc < 0) {
		puts("Failed to change process priority. You may experience lags.");
	}

	player.parse_disabled_vgm_commands(disabled_vgm_cmds);
	player.init();

	usleep(100 * 1000);

	if (test_type.empty()) {
		player.play(positional_args);
		puts("Done playing!");
	} else {
		const std::unordered_map<std::string, std::function<void()>> tests = {
			{"opl3_sine", [&](){
				printf("OPL3 Sine Wave Test\n");
				printf("From https://www.vogons.org/viewtopic.php?t=55181\n");
				puts("");

				const std::vector<std::pair<uint8_t, uint8_t>> data = {
					{0x20, 0x03},
					{0x23, 0x01},
					{0x40, 0x2f},
					{0x43, 0x00},
					{0x61, 0x10},
					{0x63, 0x10},
					{0x80, 0x00},
					{0x83, 0x00},
					{0xa0, 0x44},
					{0xb0, 0x12},
					{0xc0, 0xfe},
					{0xb0, 0x32}
				};

				for (auto &it : data) {
					printf("Write reg: 0x%02x 0x%02x\n", it.first, it.second);
					retrowave_opl3_emit_port0(&player.rtctx, it.first, it.second);
				}

				printf("Sleeping 1 sec...\n");
				sleep(1);

				const uint8_t last_reg[2] = {0x60, 0xf0};

				printf("Write reg: 0x%02x 0x%02x\n", last_reg[0], last_reg[1]);
				retrowave_opl3_emit_port0(&player.rtctx, last_reg[0], last_reg[1]);

				printf(
					"If you hear sine wave you have a real OPL3 or very accurate clone,\n"
					"otherwise you have OPL clone.\n"
					"Press Ctrl-C to close program.\n"
				);

				sleep(500);
			}
			},
		};

		auto it = tests.find(test_type);

		if (it == tests.end()) {
			printf("Available tests:\n");

			for (auto &it2 : tests) {
				std::cout << it2.first << "\n";
			}
		} else {
			std::cout << "Running test: " << it->first << "\n";
			it->second();
		}
	}


	player.do_exit(0);

	return 0;
}

