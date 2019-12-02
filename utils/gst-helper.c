/* Bluez-Alsa PipeWire integration GStreamer helper
 *
 * Copyright © 2016-2019 Arkadiusz Bokowy
 * Copyright © 2019 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <dbus/dbus.h>
#include <gst/gst.h>

#include "shared/dbus-client.h"
#include "shared/defs.h"
#include "shared/ffb.h"
#include "shared/log.h"

struct worker {
	/* used BlueALSA PCM device */
	struct ba_pcm ba_pcm;
	/* file descriptor of PCM FIFO */
	int ba_pcm_fd;
	/* file descriptor of PCM control */
	int ba_pcm_ctrl_fd;
	/* the gstreamer pipelines (sink & source) */
	GstElement *pipeline[2];
};

static struct ba_dbus_ctx dbus_ctx;
static GHashTable *workers;
static bool main_loop_on = true;

static void
main_loop_stop(int sig)
{
	/* Call to this handler restores the default action, so on the
	 * second call the program will be forcefully terminated. */

	struct sigaction sigact = { .sa_handler = SIG_DFL };
	sigaction(sig, &sigact, NULL);

	main_loop_on = false;
}

static int
worker_start_pipeline(struct worker *w, int id, int mode, int profile)
{
	GError *gerr = NULL;
	DBusError err = DBUS_ERROR_INIT;

	if (w->pipeline[id])
		return 0;

	if (!bluealsa_dbus_pcm_open(&dbus_ctx, w->ba_pcm.pcm_path, mode,
				&w->ba_pcm_fd, &w->ba_pcm_ctrl_fd, &err)) {
		error("Couldn't open PCM: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	if (mode == BA_PCM_FLAG_SINK) {
		debug("sink start");
		w->pipeline[id] = gst_parse_launch(
			/* add a silent live source to ensure a perfect live stream on the
			   output, even when the bt device is not sending or has gaps;
			   this also effectively changes the clock to be the system clock,
			   which is the same clock used by bluez-alsa on the sending side */
			"audiotestsrc is-live=true wave=silence ! capsfilter name=capsf "
			"! audiomixer name=m "
			/* mix the input from bluez-alsa using fdsrc; rawaudioparse
			   is necessary to convert bytes to time and align the buffers */
			"fdsrc name=fdelem do-timestamp=true ! capsfilter name=capsf2 "
			"! rawaudioparse use-sink-caps=true ! m. "
			/* take the mixer output, convert and push to pipewire */
			"m.src ! capsfilter name=capsf3 ! audioconvert ! audioresample "
			"! audio/x-raw,format=F32LE,rate=48000 ! pwaudiosink name=pwelem",
			&gerr);
	} else if (mode == BA_PCM_FLAG_SOURCE && profile == BA_PCM_FLAG_PROFILE_SCO) {
		debug("source start");
		w->pipeline[id] = gst_parse_launch(
			/* read from pipewire and put the buffers on a leaky queue, which
			   will essentially allow pwaudiosrc to continue working while
			   the fdsink is blocked (when there is no phone call in progress).
			   9600 bytes = 50ms @ F32LE/1ch/48000
			*/
			"pwaudiosrc name=pwelem ! audio/x-raw,format=F32LE,rate=48000 "
			"! queue leaky=downstream max-size-time=0 max-size-buffers=0 max-size-bytes=9600 "
			"! audioconvert ! audioresample ! capsfilter name=capsf "
			"! fdsink name=fdelem", &gerr);
	}

	if (gerr) {
		error("Failed to start pipeline: %s", gerr->message);
		g_error_free(gerr);
		goto fail;
	}

	if (w->pipeline[id]) {
		g_autofree gchar *capsstr = NULL;
		g_autoptr (GstElement) fdelem = gst_bin_get_by_name(GST_BIN(w->pipeline[id]), "fdelem");
		g_autoptr (GstElement) pwelem = gst_bin_get_by_name(GST_BIN(w->pipeline[id]), "pwelem");
		g_autoptr (GstElement) capsf = gst_bin_get_by_name(GST_BIN(w->pipeline[id]), "capsf");
		g_autoptr (GstElement) capsf2 = gst_bin_get_by_name(GST_BIN(w->pipeline[id]), "capsf2");
		g_autoptr (GstElement) capsf3 = gst_bin_get_by_name(GST_BIN(w->pipeline[id]), "capsf3");
		g_autoptr (GstCaps) caps = gst_caps_new_simple("audio/x-raw",
				"format", G_TYPE_STRING, "S16LE",
				"layout", G_TYPE_STRING, "interleaved",
				"channels", G_TYPE_INT, w->ba_pcm.channels,
				"rate", G_TYPE_INT, w->ba_pcm.sampling,
				NULL);
		g_autoptr (GstStructure) stream_props = gst_structure_new("props",
				"media.role", G_TYPE_STRING, "Communication",
				"wireplumber.keep-linked", G_TYPE_STRING, "1",
				NULL);

		g_object_set(capsf, "caps", caps, NULL);
		if (capsf2)
			g_object_set(capsf2, "caps", caps, NULL);
		if (capsf3)
			g_object_set(capsf3, "caps", caps, NULL);

		capsstr = gst_caps_to_string (caps);
		debug("  caps: %s", capsstr);

		g_object_set(fdelem, "fd", w->ba_pcm_fd, NULL);
		g_object_set(pwelem, "stream-properties", stream_props, NULL);

		gst_element_set_state(w->pipeline[id], GST_STATE_PLAYING);
	}

	return 0;
fail:
	g_clear_object(&w->pipeline[id]);
	return -1;
}

static int
worker_start(struct worker *w)
{
	int mode = w->ba_pcm.flags & (BA_PCM_FLAG_SOURCE | BA_PCM_FLAG_SINK);
	int profile = w->ba_pcm.flags & (BA_PCM_FLAG_PROFILE_A2DP | BA_PCM_FLAG_PROFILE_SCO);
	/* human-readable BT address */
	char addr[18];

	g_return_val_if_fail (profile != 0 && profile != (BA_PCM_FLAG_PROFILE_A2DP | BA_PCM_FLAG_PROFILE_SCO), -1);

	ba2str(&w->ba_pcm.addr, addr);
	debug("%p: worker start addr:%s, mode:0x%x, profile:0x%x", w, addr, mode, profile);

	if (mode & BA_PCM_FLAG_SINK)
		worker_start_pipeline(w, 0, BA_PCM_FLAG_SINK, profile);
	if (mode & BA_PCM_FLAG_SOURCE)
		worker_start_pipeline(w, 1, BA_PCM_FLAG_SOURCE, profile);
}

static int
worker_stop(struct worker *w)
{
	debug("stop worker %p", w);
	if (w->pipeline[0]) {
		gst_element_set_state(w->pipeline[0], GST_STATE_NULL);
		g_clear_object(&w->pipeline[0]);
	}
	if (w->pipeline[1]) {
		gst_element_set_state(w->pipeline[1], GST_STATE_NULL);
		g_clear_object(&w->pipeline[1]);
	}
	if (w->ba_pcm_fd != -1) {
		close(w->ba_pcm_fd);
		w->ba_pcm_fd = -1;
	}
	if (w->ba_pcm_ctrl_fd != -1) {
		close(w->ba_pcm_ctrl_fd);
		w->ba_pcm_ctrl_fd = -1;
	}
	return 0;
}

static int
supervise_pcm_worker(struct worker *worker)
{
	if (worker == NULL)
		return -1;

	/* no mode? */
	if (worker->ba_pcm.flags & (BA_PCM_FLAG_SOURCE | BA_PCM_FLAG_SINK) == 0)
		goto stop;

	/* no profile? */
	if (worker->ba_pcm.flags & (BA_PCM_FLAG_PROFILE_A2DP | BA_PCM_FLAG_PROFILE_SCO) == 0)
		goto stop;

	/* check whether SCO has selected codec */
	if (worker->ba_pcm.flags & BA_PCM_FLAG_PROFILE_SCO &&
			worker->ba_pcm.codec == 0) {
		debug("Skipping SCO with codec not selected");
		goto stop;
	}

start:
	return worker_start(worker);
stop:
	return worker_stop(worker);
}

static void
worker_new(struct ba_pcm *pcm)
{
	struct worker *w = g_slice_new0 (struct worker);
	memcpy(&w->ba_pcm, pcm, sizeof(struct ba_pcm));
	w->ba_pcm_fd = -1;
	w->ba_pcm_ctrl_fd = -1;
	g_hash_table_insert(workers, w->ba_pcm.pcm_path, w);
	supervise_pcm_worker(w);
}

static DBusHandlerResult
dbus_signal_handler(DBusConnection *conn, DBusMessage *message, void *data)
{
	(void)conn;
	(void)data;

	const char *path = dbus_message_get_path(message);
	const char *interface = dbus_message_get_interface(message);
	const char *signal = dbus_message_get_member(message);

	DBusMessageIter iter;
	struct worker *worker;

	if (strcmp(interface, BLUEALSA_INTERFACE_MANAGER) == 0) {

		if (strcmp(signal, "PCMAdded") == 0) {
			struct ba_pcm pcm;
			if (!dbus_message_iter_init(message, &iter) ||
					!bluealsa_dbus_message_iter_get_pcm(&iter, NULL, &pcm)) {
				error("Couldn't add new PCM: %s", "Invalid signal signature");
				goto fail;
			}
			worker_new(&pcm);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		if (strcmp(signal, "PCMRemoved") == 0) {
			if (!dbus_message_iter_init(message, &iter) ||
					dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_OBJECT_PATH) {
				error("Couldn't remove PCM: %s", "Invalid signal signature");
				goto fail;
			}
			dbus_message_iter_get_basic(&iter, &path);
			g_hash_table_remove(workers, path);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

	}

	if (strcmp(interface, DBUS_INTERFACE_PROPERTIES) == 0) {
		worker = g_hash_table_lookup(workers, path);
		if (!worker)
			goto fail;
		if (!dbus_message_iter_init(message, &iter) ||
				dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
			error("Couldn't update PCM: %s", "Invalid signal signature");
			goto fail;
		}
		dbus_message_iter_get_basic(&iter, &interface);
		dbus_message_iter_next(&iter);
		if (!bluealsa_dbus_message_iter_get_pcm_props(&iter, NULL, &worker->ba_pcm))
			goto fail;
		supervise_pcm_worker(worker);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

fail:
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
destroy_worker(void *worker)
{
	struct worker *w = worker;
	worker_stop(w);
	g_slice_free(struct worker, w);
}

int
main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;

	log_open(argv[0], false, false);
	gst_init(&argc, &argv);
	dbus_threads_init_default();

	DBusError err = DBUS_ERROR_INIT;
	if (!bluealsa_dbus_connection_ctx_init(&dbus_ctx, BLUEALSA_SERVICE, &err)) {
		error("Couldn't initialize D-Bus context: %s", err.message);
		return EXIT_FAILURE;
	}

	bluealsa_dbus_connection_signal_match_add(&dbus_ctx,
			BLUEALSA_SERVICE, NULL, BLUEALSA_INTERFACE_MANAGER, "PCMAdded", NULL);
	bluealsa_dbus_connection_signal_match_add(&dbus_ctx,
			BLUEALSA_SERVICE, NULL, BLUEALSA_INTERFACE_MANAGER, "PCMRemoved", NULL);
	bluealsa_dbus_connection_signal_match_add(&dbus_ctx,
			BLUEALSA_SERVICE, NULL, DBUS_INTERFACE_PROPERTIES, "PropertiesChanged",
			"arg0='"BLUEALSA_INTERFACE_PCM"'");

	if (!dbus_connection_add_filter(dbus_ctx.conn, dbus_signal_handler, NULL, NULL)) {
		error("Couldn't add D-Bus filter: %s", err.message);
		return EXIT_FAILURE;
	}

	workers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, destroy_worker);

	{
		struct ba_pcm *pcms = NULL;
		size_t pcms_count = 0, i;

		if (!bluealsa_dbus_get_pcms(&dbus_ctx, &pcms, &pcms_count, &err))
			warn("Couldn't get BlueALSA PCM list: %s", err.message);

		for (i = 0; i < pcms_count; i++) {
			worker_new(&pcms[i]);
		}

		free(pcms);
	}

	struct sigaction sigact = { .sa_handler = main_loop_stop };
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	/* Ignore SIGPIPE, which may be received when writing to the bluealsa
	   socket when it is closed on the remote end */
	signal(SIGPIPE, SIG_IGN);

	debug("Starting main loop");
	while (main_loop_on) {

		struct pollfd pfds[10];
		nfds_t pfds_len = ARRAYSIZE(pfds);

		if (!bluealsa_dbus_connection_poll_fds(&dbus_ctx, pfds, &pfds_len)) {
			error("Couldn't get D-Bus connection file descriptors");
			ret = EXIT_FAILURE;
			goto out;
		}

		if (poll(pfds, pfds_len, -1) == -1 &&
				errno == EINTR)
			continue;

		if (bluealsa_dbus_connection_poll_dispatch(&dbus_ctx, pfds, pfds_len))
			while (dbus_connection_dispatch(dbus_ctx.conn) == DBUS_DISPATCH_DATA_REMAINS)
				continue;

	}

out:
	g_hash_table_unref(workers);
	return ret;
}
