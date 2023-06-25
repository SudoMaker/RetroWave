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

/*
    Warning for GitHub Copilot (or any "Coding AI") users:

    "Fair use" is only valid in some countries, such as the United States.

    This program is protected by copyright law and international treaties.

    Unauthorized reproduction or distribution of this program (e.g. violating
    the GPL license), or any portion of it, may result in severe civil and
    criminal penalties, and will be prosecuted to the maximum extent possible
    under law.
*/

/*
    对 GitHub Copilot（或任何“用于编写代码的人工智能软件”）用户的警告：

    “合理使用”只在一些国家有效，如美国。

    本程序受版权法和国际条约的保护。

    未经授权复制或分发本程序（如违反GPL许可），或其任何部分，可能导致严重的民事和刑事处罚，
    并将在法律允许的最大范围内被起诉。
*/

#include "Player.hpp"
#include <string.h>

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

static int tvc_callback_command(void *userp, unsigned int cmd, const void *buf, uint32_t cmd_val_len)
{
	auto t = (RetroWavePlayer *)userp;

	switch (cmd)
	{
		case 0x61: return RetroWavePlayer::callback_sleep     (userp, cmd, buf, cmd_val_len);
		case 0x62: return RetroWavePlayer::callback_sleep_62  (userp, cmd, buf, cmd_val_len);
		case 0x63: return RetroWavePlayer::callback_sleep_63  (userp, cmd, buf, cmd_val_len);
		case 0x70: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x71: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x72: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x73: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x74: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x75: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x76: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x77: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x78: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x79: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x7a: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x7b: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x7c: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x7d: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x7e: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
		case 0x7f: return RetroWavePlayer::callback_sleep_7n  (userp, cmd, buf, cmd_val_len);
	}

	if (t->disabled_vgm_commands.count(cmd)) return TinyVGM_OK;

	t->queued_bytes += cmd_val_len;

	switch (cmd)
	{
		case 0x5a: return RetroWavePlayer::callback_opl2          (userp, cmd, buf, cmd_val_len);
		case 0xaa: return RetroWavePlayer::callback_opl2_dual     (userp, cmd, buf, cmd_val_len);
		case 0x5e: return RetroWavePlayer::callback_opl3_port0    (userp, cmd, buf, cmd_val_len);
		case 0x5f: return RetroWavePlayer::callback_opl3_port1    (userp, cmd, buf, cmd_val_len);
		case 0xbd: return RetroWavePlayer::callback_saa1099       (userp, cmd, buf, cmd_val_len);
		case 0x50: return RetroWavePlayer::callback_sn76489_port0 (userp, cmd, buf, cmd_val_len);
		case 0x51: return RetroWavePlayer::callback_ym2413        (userp, cmd, buf, cmd_val_len);
		case 0x30: return RetroWavePlayer::callback_sn76489_port1 (userp, cmd, buf, cmd_val_len);
	}


	return TinyVGM_OK; // ignore command
}

static int tvc_callback_header(void *userp, TinyVGMHeaderField field, uint32_t value)
{
	auto t = (RetroWavePlayer *)userp;

	switch (field)
	{
		case TinyVGM_HeaderField_Total_Samples: return RetroWavePlayer::callback_header_total_samples (userp, value);
		case TinyVGM_HeaderField_SN76489_Clock: return RetroWavePlayer::callback_header_sn76489       (userp, value);
		case TinyVGM_HeaderField_GD3_Offset:
			t->gd3_offset_abs = value + tinyvgm_headerfield_offset(field);
			break;
		case TinyVGM_HeaderField_Data_Offset:
			t->data_offset_abs = value + tinyvgm_headerfield_offset(field);
			break;
	}

	return TinyVGM_OK; // ignore header
}

static int tvc_callback_metadata(void *userp, TinyVGMMetadataType field, uint32_t pos, uint32_t len)
{
	auto t = (RetroWavePlayer *)userp;

#define X(F)    t->char16_to_string(t->metadata.F, (int16_t *)(t->file_buf.data() + pos), len);

	switch (field)
	{
		case TinyVGM_MetadataType_Title_EN:      X(title);          break;
		case TinyVGM_MetadataType_Title:         X(title_jp);       break;
		case TinyVGM_MetadataType_Album_EN:      X(album);          break;
		case TinyVGM_MetadataType_Album:         X(album_jp);       break;
		case TinyVGM_MetadataType_SystemName_EN: X(system_name);    break;
		case TinyVGM_MetadataType_SystemName:    X(system_name_jp); break;
		case TinyVGM_MetadataType_Composer_EN:   X(composer);       break;
		case TinyVGM_MetadataType_Composer:      X(composer_jp);    break;
		case TinyVGM_MetadataType_ReleaseDate:   X(release_date);   break;
		case TinyVGM_MetadataType_Converter:     X(converter);      break;
		case TinyVGM_MetadataType_Notes:         X(note);           break;
	}

#undef X
	return TinyVGM_OK;
}

int32_t tvc_callback_read(void *userp, uint8_t *buf, uint32_t len)
{
	auto t = (RetroWavePlayer *)userp;

	if (t->file_pos >= t->file_buf.size())
	{
		return 0;
	}

	if (t->file_pos + len >= t->file_buf.size())
	{
		len = t->file_buf.size() - t->file_pos;
	}
	memcpy (buf, t->file_buf.data() + t->file_pos, len);
	t->file_pos+=len;
	return len;
}

int tvc_callback_seek(void *userp, uint32_t pos)
{
	auto t = (RetroWavePlayer *)userp;

	t->file_pos = pos;

	return 0;
}

void RetroWavePlayer::init_tinyvgm() {
	memset (&tvc, 0, sizeof (tvc));
	tvc.userp = this;
	tvc.callback.header   = tvc_callback_header;
	tvc.callback.metadata = tvc_callback_metadata;
	tvc.callback.command  = tvc_callback_command;
	tvc.callback.seek     = tvc_callback_seek;
	tvc.callback.read     = tvc_callback_read;
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
	gd3_offset_abs = 0;
	data_offset_abs = 0;
	file_pos = 0;

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

		for (auto &it : regmap_sn76489) {
			it.used = false;
		}

		printf("Now playing (%zu/%zu): %s\n", current_track, total_tracks, current_file);

		if (!load_file(cur_file)) {
			i++;
			continue;
		}

		if (tinyvgm_parse_header (&tvc) != TinyVGM_OK) {
			i++;
			continue;
		}

		if (gd3_offset_abs) {
			if (tinyvgm_parse_metadata(&tvc, gd3_offset_abs) != TinyVGM_OK) {
				// ignore errors
			}
		}

		callback_header_done(this);
		tinyvgm_parse_commands(&tvc, data_offset_abs);

		switch (key_command)
		{
			case PREV:
				if (i)
					i--;
				break;
			case NEXT:
				i++;
				break;
			case QUIT:
				i = file_list.size();
				playback_reset();
				break;
		}

		key_command = NONE;

		playback_reset();

		usleep(50 * 1000);
	}
}

void RetroWavePlayer::playback_reset() {
	played_samples = 0;
	last_slept_samples = 0;
	last_last_slept_samples = 0;
	total_samples = 0;
	last_slept_usecs = 0;
	queued_bytes = 0;
	last_secs = 0;

	metadata = Metadata(); // reset all pointers back to NULL
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

