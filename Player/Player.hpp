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

	// Controls
	struct termios term_state;
	PlaybackCommand key_command;
	bool single_step = false;

	// RegMap
	std::map<int, std::unordered_set<uint16_t>> reg_map_refreshed_list;
	std::map<int, std::vector<uint8_t>> reg_map;
	struct {
		bool used;

		uint8_t last_reg;
		uint8_t att[3];
		uint16_t freq[3];
		uint8_t noise_att, noise_ctrl;
	} regmap_sn76489 = {0};

	// OSD
	int osd_show_regs = 1, osd_show_meta = 1;
	size_t osd_ratelimit_thresh = 1000 * 1000 * 10;
	size_t osd_ratelimited_time = 0;

	// File
	const char *current_file;
	size_t current_track = 0, total_tracks = 0;

	// Playback stats
	size_t played_samples = 0, total_samples = 0;
	bool playback_done = false;

	// Metadata
	TinyVGMGd3Info gd3_info = {0};
	struct Metadata {
		std::string title, album, system_name, composer, release_date, converter, note;
		std::string title_jp, album_jp, system_name_jp, composer_jp;
	} metadata;

	timespec sleep_end;

	TinyVGMContext tvc;
	RetroWaveContext rtctx;


public:
	static std::tuple<int, int, int> sec2hms(int _secs);
	static void set_nonblocking(int fd_, bool __nonblocking = true);

	// Main
	void init();
	void init_term();
	void init_retrowave();
	void init_tinyvgm();
	void play(const std::vector<std::string>& file_list);
	void playback_reset();

	// File I/O
	bool load_file(const std::string& path);

	// RegMap
	void regmap_insert(int idx, uint8_t reg, uint8_t val);
	void regmap_sn76489_insert(uint8_t data);

	// OSD
	static void term_clear();
	void osd_show_regmap_sn76489();
	void osd_show_regmaps();
	void osd_show_metadata();
	void osd_show();

	void do_exit(int rc);


	// Metadata
	static void char16_to_string(std::string& str, int16_t *c16);
	void gd3_to_info(TinyVGMGd3Info *g);


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

	static int callback_header_total_samples(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_header_done(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_playback_done(void *userp, uint8_t value, const void *buf, uint32_t len);

	static int callback_saa1099(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_opl2(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_opl3_port0(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_opl3_port1(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sn76489_port0(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sn76489_port1(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_ym2413(void *userp, uint8_t value, const void *buf, uint32_t len);

	static int callback_sleep(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sleep_62(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sleep_63(void *userp, uint8_t value, const void *buf, uint32_t len);
	static int callback_sleep_7n(void *userp, uint8_t value, const void *buf, uint32_t len);

	static timespec nsec_to_timespec(uint64_t nsec);
	static void timespec_add(timespec &addee, timespec adder);

	int flush_and_sleep(uint32_t sleep_samples);
};