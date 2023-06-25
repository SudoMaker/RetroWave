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

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

void RetroWavePlayer::mute_chips() {
	retrowave_opl3_mute(&rtctx);
	retrowave_mastergear_mute_sn76489(&rtctx);
	retrowave_mastergear_reset_ym2413(&rtctx);
}

void RetroWavePlayer::reset_chips() {
	retrowave_opl3_reset(&rtctx);
	retrowave_mastergear_mute_sn76489(&rtctx);
	retrowave_mastergear_reset_ym2413(&rtctx);
}

void RetroWavePlayer::flush_chips() {
	retrowave_flush(&rtctx);
}

int RetroWavePlayer::callback_header_total_samples(void *userp, uint32_t value) {
	auto *ctx = (RetroWavePlayer *)userp;

	ctx->total_samples = value;

	return TinyVGM_OK;
}

int RetroWavePlayer::callback_header_sn76489(void *userp, uint32_t value) {
	auto *ctx = (RetroWavePlayer *)userp;

	ctx->sn76489_dual = value >> 1;

	return TinyVGM_OK;
}

int RetroWavePlayer::callback_header_done(void *userp) {
	auto *ctx = (RetroWavePlayer *)userp;

	clock_gettime(RETROWAVE_PLAYER_TIME_REF, &ctx->sleep_end);

	return TinyVGM_OK;
}

int RetroWavePlayer::callback_saa1099(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 2);

	uint8_t reg = ((uint8_t *)buf)[0];
	uint8_t val = ((uint8_t *)buf)[1];

	ctx->regmap_insert(value, reg, val);
	retrowave_miniblaster_queue(&ctx->rtctx, reg, val);
	ctx->single_frame_hook();

	return TinyVGM_OK;
}

int RetroWavePlayer::callback_opl2(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 2);

	uint8_t reg = ((uint8_t *)buf)[0];
	uint8_t val = ((uint8_t *)buf)[1];

	ctx->regmap_insert(value, reg, val);
	retrowave_opl3_queue_port0(&ctx->rtctx, reg, val);
	ctx->single_frame_hook();


	return TinyVGM_OK;
}

int RetroWavePlayer::callback_opl2_dual(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 2);

	uint8_t reg = ((uint8_t *) buf)[0];
	uint8_t val = ((uint8_t *) buf)[1];

	ctx->regmap_insert(value, reg, val);
	retrowave_opl3_queue_port1(&ctx->rtctx, reg, val);
	ctx->single_frame_hook();


	return TinyVGM_OK;
}

int RetroWavePlayer::callback_opl3_port0(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 2);

	uint8_t reg = ((uint8_t *) buf)[0];
	uint8_t val = ((uint8_t *) buf)[1];

	ctx->regmap_insert(value, reg, val);
	retrowave_opl3_queue_port0(&ctx->rtctx, reg, val);
	ctx->single_frame_hook();


	return TinyVGM_OK;
}

int RetroWavePlayer::callback_opl3_port1(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 2);

	uint8_t reg = ((uint8_t *) buf)[0];
	uint8_t val = ((uint8_t *) buf)[1];

	ctx->regmap_insert(value, reg, val);
	retrowave_opl3_queue_port1(&ctx->rtctx, reg, val);
	ctx->single_frame_hook();

	return TinyVGM_OK;
}

void RetroWavePlayer::sn76489_zero_freq_workaround(uint8_t idx) {
	auto &cur_regmap = regmap_sn76489[idx];

	if (!cur_regmap.is_volume && cur_regmap.channel < 3 && cur_regmap.freq[cur_regmap.channel] == 0) {
		retrowave_mastergear_queue_sn76489(&rtctx, 0x81 | (regmap_sn76489[idx].channel << 5));
	}
}

int RetroWavePlayer::callback_sn76489_port0(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 1);

	uint8_t val = ((uint8_t *) buf)[0];

	ctx->regmap_sn76489_insert(0, val);
	ctx->sn76489_zero_freq_workaround(0);
	retrowave_mastergear_queue_sn76489(&ctx->rtctx, val);

	if (!ctx->sn76489_dual) {
		ctx->regmap_sn76489_insert(1, val);
		ctx->sn76489_zero_freq_workaround(1);
		retrowave_mastergear_queue_sn76489(&ctx->rtctx, val);
	}

	ctx->single_frame_hook();

	return TinyVGM_OK;
}

int RetroWavePlayer::callback_sn76489_port1(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 1);

	uint8_t val = ((uint8_t *) buf)[0];

	ctx->regmap_sn76489_insert(1, val);
	ctx->sn76489_zero_freq_workaround(1);
	retrowave_mastergear_queue_sn76489(&ctx->rtctx, val);

	ctx->single_frame_hook();

	return TinyVGM_OK;
}

int RetroWavePlayer::callback_ym2413(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 2);

	uint8_t reg = ((uint8_t *) buf)[0];
	uint8_t val = ((uint8_t *) buf)[1];

	ctx->regmap_insert(value, reg, val);
	retrowave_mastergear_queue_ym2413(&ctx->rtctx, reg, val);
	ctx->single_frame_hook();

	return TinyVGM_OK;
}

int RetroWavePlayer::callback_sleep(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	assert(len == 2);
	return ctx->flush_and_sleep(*((uint16_t *) buf));
}

int RetroWavePlayer::callback_sleep_62(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	return ctx->flush_and_sleep(735);
}

int RetroWavePlayer::callback_sleep_63(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	return ctx->flush_and_sleep(882);
}

int RetroWavePlayer::callback_sleep_7n(void *userp, uint8_t value, const void *buf, uint32_t len) {
	auto *ctx = (RetroWavePlayer *)userp;

	uint8_t multiply_factor = value - 0x70 + 1;
	return ctx->flush_and_sleep(multiply_factor);
}

int RetroWavePlayer::flush_and_sleep(uint32_t sleep_samples) {
	flush_chips();

	played_samples += sleep_samples;
	last_slept_samples = sleep_samples;

	uint64_t t = round(1000000000.0 * sleep_samples / sample_rate);
	last_slept_usecs = t;

	if (osd_ratelimit_thresh) {
		osd_ratelimited_time += t;

		if (osd_ratelimited_time > osd_ratelimit_thresh) {
			osd_show();
			osd_ratelimited_time = 0;
		}
	}

	int paused = 0, was_paused = 0;
	int ff = 0;

check_command:
	controls_parse_key_commands();

	switch (key_command) {
		case QUIT:
			puts("Exiting...");
			do_exit(0);
			break;
		case PAUSE_OR_PLAY:
			paused = !paused;
			was_paused = 1;
			if (paused) {
//				mute_chips();
				puts("== Paused ==");
			} else {
				puts("== Resumed ==");
				term_clear();
			}
			break;
		case NEXT:
		case PREV:
			return TinyVGM_ECANCELED;
		case SINGLE_FRAME:
			single_step = true;
			break;
		case FAST_FORWARD:
			ff = 1;
			break;
		default:
			break;
	}

	if (paused) {
		usleep(1000);
		goto check_command;
	} else {
		if (was_paused)
			clock_gettime(RETROWAVE_PLAYER_TIME_REF, &sleep_end);
	}

	if (ff) {
		key_command = NONE;
		clock_gettime(RETROWAVE_PLAYER_TIME_REF, &sleep_end);
		return TinyVGM_OK;
	}

	timespec_add(sleep_end, nsec_to_timespec(t));

#ifdef __APPLE__
	struct timespec time_now;
	clock_gettime(RETROWAVE_PLAYER_TIME_REF, &time_now);

	if (timespec_cmp(time_now, sleep_end) < 0) {
		uint64_t sleep_diff = (sleep_end.tv_sec - time_now.tv_sec) * 1000000000 + (sleep_end.tv_nsec - time_now.tv_nsec);
		mach_timebase_info_data_t timebase_info;

		mach_timebase_info(&timebase_info);
		mach_wait_until(mach_absolute_time() + sleep_diff * timebase_info.denom / timebase_info.numer);
	}
#else
	clock_nanosleep(RETROWAVE_PLAYER_TIME_REF, TIMER_ABSTIME, &sleep_end, nullptr);
#endif


	return TinyVGM_OK;
}

timespec RetroWavePlayer::nsec_to_timespec(uint64_t nsec) {
	timespec ret;
	ret.tv_sec = nsec / 1000000000;
	ret.tv_nsec = nsec % 1000000000;
	return ret;
}

void RetroWavePlayer::timespec_add(timespec &addee, const timespec &adder) {
	addee.tv_sec += adder.tv_sec;
	addee.tv_nsec += adder.tv_nsec;

	if (addee.tv_nsec > 999999999) {
		addee.tv_sec += 1;
		addee.tv_nsec -= 999999999;
	}
}

int RetroWavePlayer::timespec_cmp(const timespec &a, const timespec &b) {
	if (a.tv_sec > b.tv_sec) {
		return 1;
	} else if (a.tv_sec < b.tv_sec) {
		return -1;
	} else if (a.tv_nsec > b.tv_nsec) {
		return 1;
	} else if (a.tv_nsec < b.tv_nsec) {
		return -1;
	} else {
		return 0;
	}
}
