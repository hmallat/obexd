/*
 *
 *  OBEX Server
 *
 *  Copyright (C) 2007-2010  Nokia Corporation
 *  Copyright (C) 2007-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <glib.h>

#include "obexd.h"
#include "plugin.h"
#include "obex.h"
#include "service.h"
#include "log.h"
#include "manager.h"
#include "filesystem.h"
#include "contentfilter.h"

#define VCARD_TYPE "text/x-vcard"
#define VCARD_FILE CONFIGDIR "/vcard.vcf"

#define OPP_CHANNEL	9
#define OPP_RECORD "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>	\
<record>							\
  <attribute id=\"0x0001\">					\
    <sequence>							\
      <uuid value=\"0x1105\"/>					\
    </sequence>							\
  </attribute>							\
								\
  <attribute id=\"0x0004\">					\
    <sequence>							\
      <sequence>						\
        <uuid value=\"0x0100\"/>				\
      </sequence>						\
      <sequence>						\
        <uuid value=\"0x0003\"/>				\
        <uint8 value=\"%%u\" name=\"channel\"/>			\
      </sequence>						\
      <sequence>						\
        <uuid value=\"0x0008\"/>				\
      </sequence>						\
    </sequence>							\
  </attribute>							\
								\
  <attribute id=\"0x0009\">					\
    <sequence>							\
      <sequence>						\
        <uuid value=\"0x1105\"/>				\
        <uint16 value=\"0x%04x\" name=\"version\"/>		\
      </sequence>						\
    </sequence>							\
  </attribute>							\
								\
  <attribute id=\"0x0100\">					\
    <text value=\"%%s\" name=\"name\"/>				\
  </attribute>							\
								\
  <attribute id=\"0x0303\">					\
    <sequence>							\
%s%s%s%s%s%s%s							\
    </sequence>							\
  </attribute>							\
%s</record>"

#define OPP_RECORD_PSM						\
"  <attribute id=\"0x0200\">					\
    <uint16 value=\"%u\" name=\"psm\"/>				\
  </attribute>							\
"

static void *opp_connect(struct obex_session *os, int *err)
{
	manager_register_transfer(os);

	if (err)
		*err = 0;

	return NULL;
}

static void opp_progress(struct obex_session *os, void *user_data)
{
	manager_emit_transfer_progress(os);
}

static int opp_chkput(struct obex_session *os, void *user_data)
{
	char *folder, *name, *path;
	int32_t time;
	const char *t;
	int err;

	if (obex_get_size(os) == OBJECT_SIZE_DELETE)
		return -ENOSYS;

	t = obex_get_name(os);
	if (t != NULL && !is_filename(t))
		return -EBADR;

	if (!contentfilter_receive_file(t))
		return -EMEDIUMTYPE;

	if (obex_option_auto_accept()) {
		folder = g_strdup(obex_option_root_folder());
		name = g_strdup(obex_get_name(os));
		goto skip_auth;
	}

	time = 0;
	err = manager_request_authorization(os, time, &folder, &name);
	if (err < 0)
		return -EPERM;

	if (folder == NULL)
		folder = g_strdup(obex_option_root_folder());

	if (name == NULL)
		name = g_strdup(obex_get_name(os));

skip_auth:
	if (name == NULL || strlen(name) == 0) {
		err = -EBADR;
		goto failed;
	}

	if (g_strcmp0(name, obex_get_name(os)) != 0)
		obex_set_name(os, name);

	path = g_build_filename(folder, name, NULL);

	err = obex_put_stream_start(os, path);

	g_free(path);

	if (err < 0)
		goto failed;

	manager_emit_transfer_started(os);

failed:
	g_free(folder);
	g_free(name);

	return err;
}

static int opp_put(struct obex_session *os, void *user_data)
{
	const char *name = obex_get_name(os);
	const char *folder = obex_option_root_folder();

	if (folder == NULL)
		return -EPERM;

	if (name == NULL)
		return -EBADR;

	return 0;
}

static int opp_get(struct obex_session *os, void *user_data)
{
	const char *type;

	if (obex_get_name(os))
		return -EPERM;

	type = obex_get_type(os);

	if (type == NULL)
		return -EPERM;

	if (g_ascii_strcasecmp(type, VCARD_TYPE) == 0) {
		if (obex_get_stream_start(os, VCARD_FILE) < 0)
			return -ENOENT;

	} else
		return -EPERM;

	return 0;
}

static void opp_disconnect(struct obex_session *os, void *user_data)
{
	manager_unregister_transfer(os);
}

static void opp_reset(struct obex_session *os, void *user_data)
{
	manager_emit_transfer_completed(os);
}

static struct obex_service_driver driver = {
	.name = "Object Push server",
	.service = OBEX_OPP,
	.channel = OPP_CHANNEL,
	.record = NULL,
	.connect = opp_connect,
	.progress = opp_progress,
	.disconnect = opp_disconnect,
	.get = opp_get,
	.put = opp_put,
	.chkput = opp_chkput,
	.reset = opp_reset
};

static int opp_init(void)
{
	gboolean vcard21 = TRUE, vcard30 = TRUE, 
		vcal10 = TRUE, ical20 = TRUE,
		vnote = TRUE, vmsg = TRUE,
		any = TRUE;
	GKeyFile *config = NULL;
	GError *err = NULL;
	gchar **list = NULL;
	gchar *s = NULL;
	int i;
	int ret = 0;
	uint16_t version = 0x0100;

	ret = contentfilter_init();
	if (ret < 0)
		return ret;

	config = g_key_file_new();
	if (config == NULL)
		goto init;

	if (g_key_file_load_from_file(config, "/etc/obexd/opp.conf",
					G_KEY_FILE_NONE, NULL) == FALSE)
		goto init;

	g_key_file_set_list_separator(config, ',');

	list = g_key_file_get_string_list(config, "OPP", "DisableFormat",
					NULL, NULL);

	for (i = 0; list != NULL && list[i] != NULL; i++) {
		if (g_str_equal(list[i], "vCard2.1"))
			vcard21 = FALSE;
		else if (g_str_equal(list[i], "vCard3.0"))
			vcard30 = FALSE;
		else if (g_str_equal(list[i], "vCal1.0"))
			vcal10 = FALSE;
		else if (g_str_equal(list[i], "iCal2.0"))
			ical20 = FALSE;
		else if (g_str_equal(list[i], "vNote"))
			vnote = FALSE;
		else if (g_str_equal(list[i], "vMessage"))
			vmsg = FALSE;
		else if (g_str_equal(list[i], "any"))
			any = FALSE;
	}

	s = g_key_file_get_string(config, "OPP", "Version", &err);
	if (err) {
		DBG("opp.conf: %s", err->message);
		g_error_free(err);
		err = NULL;
	} else {
		version = strtol(s, NULL, 16);
		g_free(s);
	}

init:
	if (asprintf(&driver.record, OPP_RECORD,
			version,
			vcard21 ? "      <uint8 value=\"0x01\"/>" : "",
			vcard30 ? "      <uint8 value=\"0x02\"/>" : "",
			vcal10  ? "      <uint8 value=\"0x03\"/>" : "",
			ical20  ? "      <uint8 value=\"0x04\"/>" : "",
			vnote   ? "      <uint8 value=\"0x05\"/>" : "",
			vmsg    ? "      <uint8 value=\"0x06\"/>" : "",
			any     ? "      <uint8 value=\"0xff\"/>" : "",
			version >= 0x0102 ? OPP_RECORD_PSM : "") < 0) {
		driver.record = NULL;
		ret = -ENOMEM;
	}

	if (version >= 0x0102)
		driver.port = OBEX_PORT_RANDOM;

	g_strfreev(list);
	g_key_file_free(config);

	if (ret == 0)
		ret = obex_service_driver_register(&driver);

	return ret;
}

static void opp_exit(void)
{
	obex_service_driver_unregister(&driver);
	if (driver.record) {
		free(driver.record);
		driver.record = NULL;
	}
	contentfilter_exit();
}

OBEX_PLUGIN_DEFINE(opp, opp_init, opp_exit)
