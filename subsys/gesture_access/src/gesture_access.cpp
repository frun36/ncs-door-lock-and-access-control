/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "gesture_access/gesture_access.h"
#include "aliro/utils.h"
#include "gesture_access_model.h"

#ifdef CONFIG_DOOR_LOCK_GESTURE_ACCESS_FRAME_FORWARDING
#include "frame_forwarding.h"
#endif // CONFIG_DOOR_LOCK_GESTURE_ACCESS_FRAME_FORWARDING

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/arducam_mega.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>

LOG_MODULE_REGISTER(door_lock_gesture_access, CONFIG_DOOR_LOCK_GESTURE_ACCESS_LOG_LEVEL);

namespace DoorLock::GestureAccess {

namespace {

constexpr uint16_t kFrameWidth = Model::kInputWidth;
constexpr uint16_t kFrameHeight = Model::kInputHeight;
constexpr size_t kVideoChunkBytes = 512;
constexpr size_t kVideoBufferCount = CONFIG_VIDEO_BUFFER_POOL_NUM_MAX;

const device *sVideoDevice = DEVICE_DT_GET(DT_NODELABEL(arducam_mega));
const gpio_dt_spec sCameraLed = GPIO_DT_SPEC_GET(DT_NODELABEL(led3), gpios);

k_sem sActivateSignal;
atomic_t sActive;
uint8_t sGrayscaleFrame[Model::kInputSize];
uint32_t sDetectionCount;
bool sConfirmedDetected;

GestureDetectedCallback sDetectionCallback;

int SetVideoActive(bool active)
{
	video_control ctrl{
		.id = VIDEO_CID_ARDUCAM_LOWPOWER,
		.val = active ? 0 : 1,
	};

	int err;
	if (!active) {
		err = video_stream_stop(sVideoDevice, VIDEO_BUF_TYPE_OUTPUT);
		VerifyOrReturnStatus(err == 0, err, LOG_ERR("video_stream_stop failed (err %d)", err));
	}

	err = video_set_ctrl(sVideoDevice, &ctrl);
	VerifyOrReturnStatus(err == 0, err, LOG_ERR("video_set_ctrl failed (err %d)", err));

	if (active) {
		err = video_stream_start(sVideoDevice, VIDEO_BUF_TYPE_OUTPUT);
		VerifyOrReturnStatus(err == 0, err, LOG_ERR("video_stream_start failed (err %d)", err));
	}

	err = gpio_pin_set_dt(&sCameraLed, active);
	if (err) {
		LOG_ERR("Failed to set camera LED (err %d)", err);
		// don't return - this doesn't affect detection
	}

	LOG_INF("Video %s", active ? "activated" : "deactivated");

	return 0;
}

void ResetDebounce()
{
	sDetectionCount = 0;
	sConfirmedDetected = false;
}

void HandleDetectionResult(const Model::Result &res)
{
	LOG_INF("DETECTION | elapsed %uus | probability %u.%u%%", res.inferenceTimeUs, res.confidenceMilli / 10,
		res.confidenceMilli % 10);

	if (!res.detected) {
		sDetectionCount = 0;
		sConfirmedDetected = false;
		return;
	}

	// React only to detection state rising edge
	VerifyOrReturn(!sConfirmedDetected);

	sDetectionCount++;
	LOG_INF("DETECTION | OK (count %u)", sDetectionCount);
	if (sDetectionCount >= CONFIG_DOOR_LOCK_GESTURE_ACCESS_DEBOUNCE_FRAMES) {
		sConfirmedDetected = true;
		VerifyAndCall(sDetectionCallback);
	}
}

void ExtractGrayscale(const video_buffer &vbuf, size_t &grayscaleBytesFilled)
{
	const size_t samples = MIN(vbuf.bytesused / 2, Model::kInputSize - grayscaleBytesFilled);

	for (size_t i = 0; i < samples; i++) {
		sGrayscaleFrame[grayscaleBytesFilled + i] = vbuf.buffer[i * 2];
	}

	grayscaleBytesFilled += samples;
}

#ifdef CONFIG_DOOR_LOCK_GESTURE_ACCESS_FRAME_FORWARDING
void ForwardFrameIfHostReady(const Model::Result &result)
{
	VerifyOrReturn(FrameForwarding::HostReady());

	char meta[256];
	int offset = snprintf(meta, sizeof(meta), "{\"det\":%d,\"conf\":%u,\"us\":%u,\"pts\":[", result.detected,
			      result.confidenceMilli, result.inferenceTimeUs);

	for (size_t i = 0; i < result.detectionCount && offset > 0 && (size_t)offset < sizeof(meta); i++) {
		offset += snprintf(&meta[offset], sizeof(meta) - (size_t)offset, "%s{\"x\":%u,\"y\":%u,\"conf\":%u}",
				   i ? "," : "", result.detections[i].x, result.detections[i].y,
				   result.detections[i].confidenceMilli);
	}

	if (offset > 0 && (size_t)offset < sizeof(meta) - 2) {
		offset += snprintf(&meta[offset], sizeof(meta) - (size_t)offset, "]}");
	}

	if (offset > 0 && (size_t)offset < sizeof(meta)) {
		FrameForwarding::Send(kFrameWidth, kFrameHeight, sGrayscaleFrame, meta, static_cast<size_t>(offset));
	}
}
#endif // CONFIG_DOOR_LOCK_GESTURE_ACCESS_FRAME_FORWARDING

int CaptureAndInferOneFrame()
{
	size_t grayscaleBytesFilled = 0;

	while (grayscaleBytesFilled < Model::kInputSize) {
		video_buffer *vbuf;
		int err = video_dequeue(sVideoDevice, &vbuf, K_MSEC(1000));

		VerifyOrReturnStatus(err == 0, err, LOG_ERR("video_dequeue failed (err %d)", err));

		ExtractGrayscale(*vbuf, grayscaleBytesFilled);

		err = video_enqueue(sVideoDevice, vbuf);
		VerifyOrReturnStatus(err == 0, err, LOG_ERR("video_enqueue failed (err %d)", err));
	}

	Model::Result result;
	int err = Model::Run(sGrayscaleFrame, Model::kInputSize, result);
	VerifyOrReturnStatus(err == 0, err, LOG_ERR("Model::Run failed (err %d)", err));

	HandleDetectionResult(result);

#ifdef CONFIG_DOOR_LOCK_GESTURE_ACCESS_FRAME_FORWARDING
	ForwardFrameIfHostReady(result);
#endif // CONFIG_DOOR_LOCK_GESTURE_ACCESS_FRAME_FORWARDING

	return 0;
}

void CaptureThreadFn(void *, void *, void *)
{
	for (;;) {
		k_sem_take(&sActivateSignal, K_FOREVER);
		if (!atomic_get(&sActive)) {
			continue;
		}

		ResetDebounce();

		int err = SetVideoActive(true);
		if (err) {
			LOG_ERR("Failed to activate video (err %d)", err);
			continue;
		}

		while (atomic_get(&sActive)) {
			CaptureAndInferOneFrame();
		}

		err = SetVideoActive(false);
		if (err) {
			LOG_ERR("Failed to deactivate video (err %d)", err);
			continue;
		}
	}
}

K_THREAD_DEFINE(sCaptureThread, CONFIG_DOOR_LOCK_GESTURE_ACCESS_THREAD_STACK_SIZE, CaptureThreadFn, NULL, NULL, NULL,
		CONFIG_DOOR_LOCK_GESTURE_ACCESS_THREAD_PRIORITY, 0, K_TICKS_FOREVER);

} // namespace

int Init(GestureDetectedCallback callback)
{
	sDetectionCallback = callback;

	LOG_INF("Gesture access init");

	VerifyOrReturnStatus(gpio_is_ready_dt(&sCameraLed), -ENODEV, LOG_ERR("Camera LED is not ready"));

	int err = gpio_pin_configure_dt(&sCameraLed, GPIO_OUTPUT_INACTIVE);
	VerifyOrReturnStatus(err == 0, err, LOG_ERR("Failed to configure camera LED (err %d)", err));

	k_sem_init(&sActivateSignal, 0, 1);

	VerifyOrReturnStatus(device_is_ready(sVideoDevice), -ENODEV, LOG_ERR("Video device not ready"));

	video_format fmt{
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = VIDEO_PIX_FMT_YUYV,
		.width = kFrameWidth,
		.height = kFrameHeight,
		.pitch = static_cast<uint32_t>(kFrameWidth * 2),
	};

	err = video_set_format(sVideoDevice, &fmt);
	VerifyOrReturnStatus(err == 0, err, LOG_ERR("Failed to set video format (err %d)", err));

	for (size_t i = 0; i < kVideoBufferCount; i++) {
		video_buffer *vbuf = video_buffer_alloc(kVideoChunkBytes, K_NO_WAIT);

		VerifyOrReturnStatus(vbuf != nullptr, -ENOMEM, LOG_ERR("Failed to allocate video buffer %zu", i));

		vbuf->type = VIDEO_BUF_TYPE_OUTPUT;
		video_enqueue(sVideoDevice, vbuf);
	}

	err = Model::Init();
	VerifyOrReturnStatus(err == 0, err, LOG_ERR("Failed to init model (err %d)", err));

#ifdef CONFIG_DOOR_LOCK_GESTURE_ACCESS_FRAME_FORWARDING
	err = FrameForwarding::Init();
	if (err) {
		LOG_WRN("Frame forwarding unavailable (err %d)", err);
	}
#endif // CONFIG_DOOR_LOCK_GESTURE_ACCESS_FRAME_FORWARDING

	k_thread_start(sCaptureThread);

	return 0;
}

void SetDetectionActive(bool active)
{
	atomic_set(&sActive, active);
	k_sem_give(&sActivateSignal);
}

} // namespace DoorLock::GestureAccess
