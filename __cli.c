/*
   Copyright (C) 2009 - 2010 Artem Makhutov
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
*/

static char* cli_show_devices (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	pvt_t* pvt;

#define FORMAT1 "%-12.12s %-5.5s %-10.10s %-5.5s %-3.3s %-4.4s %-4.4s %-7.7s %-14.14s %-10.10s %-17.17s %-17.17s %-14.14s\n"
#define FORMAT2 "%-12.12s %-5d %-10.10s %-5.5s %-3.3s %-4d %-4d %-7d %-14.14s %-10.10s %-17.17s %-17.17s %-14.14s\n"

	switch (cmd)
	{
		case CLI_INIT:
			e->command =	"datacard show devices";
			e->usage   =	"Usage: datacard show devices\n"
					"       Shows the state of Datacard devices.\n";
			return NULL;

		case CLI_GENERATE:
			return NULL;
	}

	if (a->argc < 3)
	{
		return CLI_SHOWUSAGE;
	}

	ast_cli (a->fd, FORMAT1, "ID", "Group", "State", "Voice", "SMS", "RSSI", "Mode", "Submode", "Provider Name", "Model", "Firmware", "IMEI", "Number");

	AST_RWLIST_RDLOCK (&devices);
	AST_RWLIST_TRAVERSE (&devices, pvt, entry)
	{
		ast_mutex_lock (&pvt->lock);
		ast_cli (a->fd, FORMAT2,
			pvt->id,
			pvt->group,

			(!pvt->connected) ? "Not connected" :
			(!pvt->initialized) ? "Not initialized" :
			(!pvt->gsm_registered) ? "GSM not registered" :
			(pvt->outgoing || pvt->incoming) ? "Busy" :
			(pvt->outgoing_sms || pvt->incoming_sms) ? "SMS" : "Free",

			(pvt->has_voice) ? "Yes" : "No",
			(pvt->has_sms) ? "Yes" : "No",
			pvt->rssi,
			pvt->linkmode,
			pvt->linksubmode,
			pvt->provider_name,
			pvt->model,
			pvt->firmware,
			pvt->imei,
			pvt->number
		);
		ast_mutex_unlock (&pvt->lock);
	}
	AST_RWLIST_UNLOCK (&devices);

#undef FORMAT1
#undef FORMAT2

	return CLI_SUCCESS;
}

static char* cli_show_device (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	pvt_t* pvt;

	switch (cmd)
	{
		case CLI_INIT:
			e->command =	"datacard show device";
			e->usage   =	"Usage: datacard show device <device>\n"
					"       Shows the state and config of Datacard device.\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 3)
			{
				return complete_device (a->line, a->word, a->pos, a->n, 0);
			}
			return NULL;
	}

	if (a->argc < 4)
	{
		return CLI_SHOWUSAGE;
	}

	pvt = find_device (a->argv[3]);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		ast_cli (a->fd, "\n Current device settings:\n");
		ast_cli (a->fd, "------------------------------------\n");
		ast_cli (a->fd, "  Device                  : %s\n", pvt->id);
		ast_cli (a->fd, "  Group                   : %d\n", pvt->group);
		ast_cli (a->fd, "  State                   : %s\n",
			(!pvt->connected) ? "Not connected" :
			(!pvt->initialized) ? "Not initialized" :
			(!pvt->gsm_registered) ? "GSM not registered" :
			(pvt->outgoing || pvt->incoming) ? "Busy" :
			(pvt->outgoing_sms || pvt->incoming_sms) ? "SMS" : "Free"
		);
		ast_cli (a->fd, "  Voice                   : %s\n", (pvt->has_voice) ? "Yes" : "No");
		ast_cli (a->fd, "  SMS                     : %s\n", (pvt->has_sms) ? "Yes" : "No");
		ast_cli (a->fd, "  RSSI                    : %d\n", pvt->rssi);
		ast_cli (a->fd, "  Mode                    : %d\n", pvt->linkmode);
		ast_cli (a->fd, "  Submode                 : %d\n", pvt->linksubmode);
		ast_cli (a->fd, "  ProviderName            : %s\n", pvt->provider_name);
		ast_cli (a->fd, "  Manufacturer            : %s\n", pvt->manufacturer);
		ast_cli (a->fd, "  Model                   : %s\n", pvt->model);
		ast_cli (a->fd, "  Firmware                : %s\n", pvt->firmware);
		ast_cli (a->fd, "  IMEI                    : %s\n", pvt->imei);
		ast_cli (a->fd, "  Number                  : %s\n", pvt->number);
		ast_cli (a->fd, "  Use CallingPres         : %s\n", pvt->usecallingpres ? "Yes" : "No");
		ast_cli (a->fd, "  Default CallingPres     : %s\n", pvt->callingpres < 0 ? "<Not set>" : ast_describe_caller_presentation (pvt->callingpres));
		ast_cli (a->fd, "  Use UCS-2 encoding      : %s\n", pvt->use_ucs2_encoding ? "Yes" : "No");
		ast_cli (a->fd, "  USSD use 7 bit encoding : %s\n", pvt->cusd_use_7bit_encoding ? "Yes" : "No");
		ast_cli (a->fd, "  USSD use UCS-2 decoding : %s\n", pvt->cusd_use_ucs2_decoding ? "Yes" : "No");
		ast_cli (a->fd, "  USSD use UCS-2 decoding : %s\n", pvt->cusd_use_ucs2_decoding ? "Yes" : "No");
		ast_cli (a->fd, "  Location area code      : %s\n", pvt->location_area_code);
		ast_cli (a->fd, "  Cell ID                 : %s\n\n", pvt->cell_id);
		ast_mutex_unlock (&pvt->lock);
	}
	else
	{
		ast_cli (a->fd, "Device %s not found\n", a->argv[2]);
	}

	return CLI_SUCCESS;
}

static char* cli_cmd (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	pvt_t*	pvt = NULL;
	char	buf[1024];

	switch (cmd)
	{
		case CLI_INIT:
			e->command =	"datacard cmd";
			e->usage   =	"Usage: datacard cmd <device> <command>\n"
					"       Send <command> to the rfcomm port on the device\n"
					"       with the specified <device>.\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 2)
			{
				return complete_device (a->line, a->word, a->pos, a->n, 0);
			}
			return NULL;
	}

	if (a->argc < 4)
	{
		return CLI_SHOWUSAGE;
	}

	pvt = find_device (a->argv[2]);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		if (pvt->connected)
		{
			snprintf (buf, sizeof (buf), "%s\r", a->argv[3]);
			if (at_write (pvt, buf) || at_fifo_queue_add (pvt, CMD_UNKNOWN, RES_OK))
			{
				ast_log (LOG_ERROR, "[%s] Error sending command: %s\n", pvt->id, a->argv[3]);
			}
		}
		else
		{
			ast_cli (a->fd, "Device %s not connected\n", a->argv[2]);
		}
		ast_mutex_unlock (&pvt->lock);
	}
	else
	{
		ast_cli (a->fd, "Device %s not found\n\n", a->argv[2]);
	}

	return CLI_SUCCESS;
}

static char* cli_ussd (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	pvt_t* pvt = NULL;

	switch (cmd)
	{
		case CLI_INIT:
			e->command = "datacard ussd";
			e->usage =
				"Usage: datacard ussd <device> <command>\n"
				"       Send ussd <command> to the datacard\n"
				"       with the specified <device>.\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 2)
			{
				return complete_device (a->line, a->word, a->pos, a->n, 0);
			}
			return NULL;
	}

	if (a->argc < 4)
	{
		return CLI_SHOWUSAGE;
	}

	pvt = find_device (a->argv[2]);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		if (pvt->connected && pvt->initialized && pvt->gsm_registered)
		{
			if (at_send_cusd (pvt, a->argv[3]) || at_fifo_queue_add (pvt, CMD_AT_CUSD, RES_OK))
			{
				ast_log (LOG_ERROR, "[%s] Error sending USSD command\n", pvt->id);
			}
		}
		else
		{
			ast_cli (a->fd, "Device %s not connected / initialized / registered\n", a->argv[2]);
		}
		ast_mutex_unlock (&pvt->lock);
	}
	else
	{
		ast_cli (a->fd, "Device %s not found\n", a->argv[2]);
	}

	return CLI_SUCCESS;
}

static char* cli_sms (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	pvt_t*	pvt = NULL;
	char*	msg;
	struct ast_str*	buf;
	int	i;

	switch (cmd)
	{
		case CLI_INIT:
			e->command = "datacard sms";
			e->usage =
				"Usage: datacard sms <device ID> <number> <message>\n"
				"       Send a sms to <number> with the <message>\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 2)
			{
				return complete_device (a->line, a->word, a->pos, a->n, 0);
			}
			return NULL;
	}

	if (a->argc < 5)
	{
		return CLI_SHOWUSAGE;
	}

	pvt = find_device (a->argv[2]);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		if (pvt->connected && pvt->initialized && pvt->gsm_registered)
		{
			if (pvt->has_sms)
			{
				buf = ast_str_create (256);

				for (i = 4; i < a->argc; i++)
				{
					if (i < (a->argc - 1))
					{
						ast_str_append (&buf, 0, "%s ", a->argv[i]);
					}
					else
					{
						ast_str_append (&buf, 0, "%s", a->argv[i]);
					}
				}

				msg = ast_strdup (ast_str_buffer (buf));

				if (at_send_cmgs (pvt, a->argv[3]) || at_fifo_queue_add_ptr (pvt, CMD_AT_CMGS, RES_SMS_PROMPT, msg))
				{
					ast_free (msg);
					ast_log (LOG_ERROR, "[%s] Error sending SMS message\n", pvt->id);
				}
				else
				{
					ast_cli (a->fd, "[%s] SMS send successful\n", pvt->id);
				}

				ast_free (buf);
			}
			else
			{
				ast_cli (a->fd, "Device %s doesn't handle SMS -- SMS will not be sent\n", pvt->id);
			}
		}
		else
		{
			ast_cli (a->fd, "Device %s not connected / initialized / registered -- SMS will not be sent\n", pvt->id);
		}
		ast_mutex_unlock (&pvt->lock);
	}
	else
	{
		ast_cli (a->fd, "Device %s not found\n", a->argv[2]);
	}

	return CLI_SUCCESS;
}
