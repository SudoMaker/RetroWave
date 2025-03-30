/*
    This file is part of RetroWave.

    Copyright (C) 2025 Christian Kündig <christian@kuendig.info>

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


#include "Web_SerialPort.h"
#include "assert.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>

static const char log_tag[] = "retrowave platform web_serialport";

EM_ASYNC_JS(ssize_t, webserial_write, (const void * __buf,size_t __nbyte	), { // ssize_t (aka long), const void * __buf, size_t __nbyte (aka unsigned long)
	const port = globalThis['retrowave_port'];

	if ('retrowave_writer' in globalThis === false) {
		console.log("Getting Writer");
		globalThis['retrowave_writer'] = port.writable.getWriter();
	}
	data = new DataView(HEAPU8.buffer, __buf, __nbyte);
	await globalThis['retrowave_writer'].write(data);
	return __nbyte;
});

static void io_callback(void *userp, uint32_t data_rate, const void *tx_buf, void *rx_buf, uint32_t len) {
	RetroWavePlatform_WebSerialPort *ctx = userp;

	uint32_t packed_len = retrowave_protocol_serial_packed_length(len);
	uint8_t *packed_data;

	if (packed_len > 128)
		packed_data = malloc(packed_len);
	else
		packed_data = alloca(packed_len);
	assert(retrowave_protocol_serial_pack(tx_buf, len, packed_data) == packed_len);

	size_t written = 0;
	while (written < len) {
		ssize_t rc = webserial_write(packed_data + written, packed_len - written);
		emscripten_sleep(1);
		if (rc > 0) {
			written += rc;
		} else {
			fprintf(stderr, "%s: FATAL: failed to write to tty: %s\n", log_tag, strerror(errno));
			abort();
		}
	}

	if (packed_len > 128)
		free(packed_data);
}

EM_ASYNC_JS(int, webserial_deinit, (), {
	globalThis['retrowave_writer'].releaseLock();
	await globalThis['retrowave_port'].close();
	delete globalThis['retrowave_writer'];
	delete globalThis['retrowave_port'];
});

EM_ASYNC_JS(int, webserial_init, (), {
	const supported = !!navigator.serial;
	if(!supported) {
		  console.error("Serial API not supported");
	  return -1;
	}
	
	try {
		const port = await navigator.serial.requestPort(); //TODO: filter for pid:"e683", vid: "04d8" (check if other revision have different values)
			
		console.log(port);
		globalThis['retrowave_port'] = port;
		await port.open({ baudRate: 115200 });
		
		const info = port.getInfo();
		vid=info.usbVendorId;
		pid= info.usbProductId;
		console.log(info);
	} catch (e) {
		console.error("Error requesting port: ", e);
		return -1;
	}
	try {
		const ports = await navigator.serial.getPorts();
		const id = vid + ':' + pid;
		//port_new = ports.find((port) => vid_pid(port) === id);
		console.log(ports);
	} catch (e) {
		console.error("Error getting ports: ", e);
		return -1;
	}
	
	return 0;
});
	  
	  
int retrowave_init_web_serialport(RetroWaveContext *ctx) {
	retrowave_init(ctx);

	ctx->user_data = malloc(sizeof(RetroWavePlatform_WebSerialPort));
	RetroWavePlatform_WebSerialPort *pctx = ctx->user_data;

	if(webserial_init() != 0) {
		fprintf(stderr, "%s: FATAL: failed to initialize web serial port\n", log_tag);
		return -1;
	}

	ctx->callback_io = io_callback;
	return 0;
}

void retrowave_deinit_web_serialport(RetroWaveContext *ctx) {
	webserial_deinit();
}

#endif