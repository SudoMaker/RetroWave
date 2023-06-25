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

#pragma once

#include <iostream>
#include <thread>
#include <system_error>
#include <locale>
#include <codecvt>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>
#include <csignal>
#include <cinttypes>

#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <termios.h>

#include <zlib.h>

#include <RetroWaveLib/RetroWave.h>
#include <RetroWaveLib/Platform/Linux_SPI.h>
#include <RetroWaveLib/Platform/POSIX_SerialPort.h>
#include <RetroWaveLib/Board/OPL3.h>
#include <RetroWaveLib/Board/MiniBlaster.h>
#include <RetroWaveLib/Board/MasterGear.h>

#include <TinyVGM.h>

#define CXXOPTS_VECTOR_DELIMITER '\0'
#include <cxxopts.hpp>

#if defined (__CYGWIN__)
#define RETROWAVE_PLAYER_TIME_REF	CLOCK_REALTIME
#else
#define RETROWAVE_PLAYER_TIME_REF	CLOCK_MONOTONIC
#endif


typedef struct {
	bool used;

	bool is_latch, is_volume;
	uint8_t channel, data;

	uint8_t last_reg;

	uint8_t noise_ctrl;
	uint8_t att[4];
	uint16_t freq[3];

} SN76489Registers;

class RetroWavePlayer {
public:
	enum {
		sample_rate = 44100
	};

	enum PlaybackCommand {
		NONE = 0x0,
		QUIT = 0x1, PAUSE_OR_PLAY = 0x2,
		NEXT = 0x4, PREV = 0x8,
		SINGLE_FRAME = 0x10,
		FAST_FORWARD = 0x20
	};

	// File I/O
	std::vector<uint8_t> file_buf;
	uint32_t file_pos;

	// Controls
	struct termios term_state;
	PlaybackCommand key_command;
	bool single_step = false;

	// RegMap
	std::map<int, std::unordered_set<uint16_t>> reg_map_refreshed_list;
	std::map<int, std::vector<uint8_t>> reg_map;

	SN76489Registers regmap_sn76489[2]{};

	// OSD
	int osd_show_regs = 1, osd_show_meta = 1;
	size_t osd_ratelimit_thresh = 1000 * 1000 * 10;
	size_t osd_ratelimited_time = 0;

	// File
	const char *current_file;
	size_t current_track = 0, total_tracks = 0;

	// Playback stats
	size_t played_samples = 0, last_slept_samples = 0, last_last_slept_samples = 0, total_samples = 0;
	size_t queued_bytes = 0, last_secs = 0, bytes_per_sec = 0;
	uint64_t last_slept_usecs = 0;
	bool sn76489_dual = false;

	// Metadata
	struct Metadata {
		std::string title, album, system_name, composer, release_date, converter, note;
		std::string title_jp, album_jp, system_name_jp, composer_jp;
	} metadata;

	timespec sleep_end;

	TinyVGMContext tvc;
	uint32_t gd3_offset_abs;
	uint32_t data_offset_abs;
	RetroWaveContext rtctx;

	std::unordered_set<uint8_t> disabled_vgm_commands;


public:
	static std::tuple<size_t, size_t, size_t> sec2hms(size_t _secs);
	static void set_nonblocking(int fd_, bool __nonblocking = true);

	// Main
	void init();
	void init_term();
	void init_retrowave();
	void init_tinyvgm();
	void play(const std::vector<std::string>& file_list);
	void playback_reset();
	void parse_disabled_vgm_commands(const std::string &str);

	// File I/O
	bool load_file(const std::string& path);

	// RegMap
	void regmap_insert(int idx, uint8_t reg, uint8_t val);
	void regmap_sn76489_insert(uint8_t chip_idx, uint8_t data);

	// OSD
	static void term_clear();
	static void term_move_0_0();
	void osd_show_regmap_sn76489();
	void osd_show_regmaps();
	void osd_show_metadata();
	void osd_show();

	void do_exit(int rc);


	// Metadata
	static void char16_to_string(std::string& str, int16_t *c16, uint32_t memsize);

	// Controls
	static void term_attr_disable_buffering();
	void term_attr_save();
	void term_attr_load();
	static int term_read_char();
	void controls_parse_key_commands();
	void single_frame_hook();


	// SoundDriver
	void mute_chips();
	void reset_chips();
	void flush_chips();

	static int callback_header_total_samples(void *userp, uint32_t value);
	static int callback_header_sn76489(void *userp, uint32_t value);
	static int callback_header_done(void *userp);

	static int callback_saa1099(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_opl2(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_opl2_dual(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_opl3_port0(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_opl3_port1(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sn76489_port0(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sn76489_port1(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_ym2413(void *userp, uint8_t value, const void *buf, uint32_t len);

	static int callback_sleep(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sleep_62(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sleep_63(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sleep_7n(void *userp, uint8_t value, const void *buf, uint32_t len);

	void sn76489_zero_freq_workaround(uint8_t idx);

	static timespec nsec_to_timespec(uint64_t nsec);
	static void timespec_add(timespec &addee, const timespec &adder);
	static int timespec_cmp(const timespec &a, const timespec &b);

	int flush_and_sleep(uint32_t sleep_samples);
};
