/*!
 * \brief Do response
 * \param pvt -- pvt structure
 * \param iovcnt -- number of elements array pvt->read_iov
 * \param at_res -- result type
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response (pvt_t* pvt, int iovcnt, at_res_t at_res)
{
	char*		str;
	size_t		len;
	at_queue_t*	e;

	if (iovcnt > 0)
	{
		len = pvt->read_iov[0].iov_len + pvt->read_iov[1].iov_len - 1;

		if (iovcnt == 2)
		{
			ast_debug (5, "[%s] iovcnt == 2\n", pvt->id);

			if (len > sizeof (pvt->parse_buf) - 1)
			{
				ast_debug (1, "[%s] buffer overflow\n", pvt->id);
				return -1;
			}

			str = pvt->parse_buf;
			memmove (str, pvt->read_iov[0].iov_base, pvt->read_iov[0].iov_len);
			memmove (str + pvt->read_iov[0].iov_len, pvt->read_iov[1].iov_base, pvt->read_iov[1].iov_len);
			str[len] = '\0';
		}
		else
		{
			str = pvt->read_iov[0].iov_base;
			str[len] = '\0';
		}

		ast_debug (5, "[%s] [%.*s]\n", pvt->id, (int) len, str);

		switch (at_res)
		{
			case RES_BOOT:
			case RES_CONF:
			case RES_CSSI:
			case RES_CSSU:
			case RES_SRVST:
				return 0;

			case RES_OK:
				return at_response_ok (pvt);

			case RES_RSSI:
				return at_response_rssi (pvt, str, len);

			case RES_MODE:
				/* An error here is not fatal. Just keep going. */
				at_response_mode (pvt, str, len);
				return 0;

			case RES_ORIG:
				return at_response_orig (pvt, str, len);

			case RES_CEND:
				return at_response_cend (pvt, str, len);

			case RES_CONN:
				return at_response_conn (pvt);

			case RES_CREG:
				/* An error here is not fatal. Just keep going. */
				at_response_creg (pvt);
				return 0;

			case RES_COPS:
				/* An error here is not fatal. Just keep going. */
				at_response_cops (pvt, str, len);
				return 0;



			case RES_CSQ:
				return at_response_csq (pvt, str, len);

			case RES_CMS_ERROR:
			case RES_ERROR:
				return at_response_error (pvt);

			case RES_RING:
				return at_response_ring (pvt);

			case RES_SMMEMFULL:
				return at_response_smmemfull (pvt);

			case RES_CLIP:
				return at_response_clip (pvt, str, len);

			case RES_CMTI:
				return at_response_cmti (pvt, str, len);

			case RES_CMGR:
				return at_response_cmgr (pvt, str, len);

			case RES_SMS_PROMPT:
				return at_response_sms_prompt (pvt);

			case RES_CUSD:
				return at_response_cusd (pvt, str, len);

			case RES_BUSY:
				return at_response_busy (pvt);

			case RES_NO_DIALTONE:
				return at_response_no_dialtone (pvt);

			case RES_NO_CARRIER:
				return at_response_no_carrier (pvt);

			case RES_CPIN:
				return at_response_cpin (pvt, str, len);

			case RES_CNUM:
				/* An error here is not fatal. Just keep going. */
				at_response_cnum (pvt, str, len);
				return 0;

			case RES_PARSE_ERROR:
				ast_debug (1, "[%s] Error parsing message\n", pvt->id);
				return -1;

			case RES_UNKNOWN:
				if ((e = at_fifo_queue_head (pvt)))
				{
					switch (e->cmd)
					{
						case CMD_AT_CGMI:
							ast_debug (1, "[%s] Got AT_CGMI data (manufacturer info)\n", pvt->id);
							return at_response_cgmi (pvt, str, len);

						case CMD_AT_CGMM:
							ast_debug (1, "[%s] Got AT_CGMM data (model info)\n", pvt->id);
							return at_response_cgmm (pvt, str, len);

						case CMD_AT_CGMR:
							ast_debug (1, "[%s] Got AT+CGMR data (firmware info)\n", pvt->id);
							return at_response_cgmr (pvt, str, len);

						case CMD_AT_CGSN:
							ast_debug (1, "[%s] Got AT+CGSN data (IMEI number)\n", pvt->id);
							return at_response_cgsn (pvt, str, len);

						default:
							ast_debug (1, "[%s] Ignoring unknown result: '%.*s'\n", pvt->id, (int) len, str);
							break;
					}
				}
				else
				{
					ast_debug (1, "[%s] Ignoring unknown result: '%.*s'\n", pvt->id, (int) len, str);
				}

				break;
		}
	}

	return 0;
}

/*!
 * \brief Handle OK response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_ok (pvt_t* pvt)
{
	at_queue_t* e;

	if ((e = at_fifo_queue_head (pvt)) && e->res == RES_OK)
	{
		switch (e->cmd)
		{
			/* initilization stuff */
			case CMD_AT:
				if (!pvt->initialized)
				{
					if (pvt->reset_datacard == 1)
					{
						if (at_send_atz (pvt) || at_fifo_queue_add (pvt, CMD_AT_Z, RES_OK))
						{
							ast_debug (1, "[%s] Error reset datacard\n", pvt->id);
							goto e_return;
						}
					}
					else
					{
						if (at_send_ate0 (pvt) || at_fifo_queue_add (pvt, CMD_AT_E, RES_OK))
						{
							ast_debug (1, "[%s] Error disabling echo\n", pvt->id);
							goto e_return;
						}
					}
				}
				break;

			case CMD_AT_Z:
				if (at_send_ate0 (pvt) || at_fifo_queue_add (pvt, CMD_AT_E, RES_OK))
				{
					ast_debug (1, "[%s] Error disabling echo\n", pvt->id);
					goto e_return;
				}
				break;

			case CMD_AT_E:
				if (!pvt->initialized)
				{
					if (pvt->u2diag != -1)
					{
						if (at_send_u2diag (pvt, pvt->u2diag) || at_fifo_queue_add (pvt, CMD_AT_U2DIAG, RES_OK))
						{
							ast_debug (1, "[%s] Error setting U2DIAG\n", pvt->id);
							goto e_return;
						}
					}
					else
					{
						if (at_send_cgmi (pvt) || at_fifo_queue_add (pvt, CMD_AT_CGMI, RES_OK))
						{
							ast_debug (1, "[%s] Error asking datacard for vendor info\n", pvt->id);
							goto e_return;
						}
					}
				}
				break;

			case CMD_AT_U2DIAG:
				if (!pvt->initialized)
				{
					if (at_send_cgmi (pvt) || at_fifo_queue_add (pvt, CMD_AT_CGMI, RES_OK))
					{
						ast_debug (1, "[%s] Error asking datacard for vendor info\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CGMI:
				if (!pvt->initialized)
				{
					if (at_send_cgmm (pvt) || at_fifo_queue_add (pvt, CMD_AT_CGMM, RES_OK))
					{
						ast_debug(1, "[%s] Error asking datacard for manufacturer\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CGMM:
				if (!pvt->initialized)
				{
					if (at_send_cgmr (pvt) || at_fifo_queue_add (pvt, CMD_AT_CGMR, RES_OK))
					{
						ast_debug (1, "[%s] Error asking datacard for model\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CGMR:
				if (!pvt->initialized)
				{
					if (at_send_cgsn (pvt) || at_fifo_queue_add (pvt, CMD_AT_CGSN, RES_OK))
					{
						ast_debug (1, "[%s] Error asking datacard for firmware\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CGSN:
				if (!pvt->initialized)
				{
					if (at_send_cpin_test (pvt) || at_fifo_queue_add (pvt, CMD_AT_CPIN, RES_OK))
					{
						ast_debug (1, "[%s] Error asking datacard for IMEI number\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CPIN:
				if (!pvt->initialized)
				{
					if (at_send_cops_init (pvt, 0, 0) || at_fifo_queue_add (pvt, CMD_AT_COPS_INIT, RES_OK))
					{
						ast_debug (1, "[%s] Error setting operator select parameters\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_COPS_INIT:
				ast_debug (1, "[%s] Operator select parameters set\n", pvt->id);

				if (!pvt->initialized)
				{
					if (at_send_creg_init (pvt, 2) || at_fifo_queue_add (pvt, CMD_AT_CREG_INIT, RES_OK))
					{
						ast_debug (1, "[%s] Error enabeling registration info\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CREG_INIT:
				ast_debug (1, "[%s] registration info enabled\n", pvt->id);

				if (!pvt->initialized)
				{
					if (at_send_creg (pvt) || at_fifo_queue_add (pvt, CMD_AT_CREG, RES_OK))
					{
						ast_debug (1, "[%s] Error sending registration query\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CREG:
				ast_debug (1, "[%s] registration query sent\n", pvt->id);

				if (!pvt->initialized)
				{
					if (at_send_cnum (pvt) || at_fifo_queue_add (pvt, CMD_AT_CNUM, RES_OK))
					{
						ast_debug (1, "[%s] Error checking subscriber phone number\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CNUM:
				ast_debug (1, "[%s] Subscriber phone number query successed\n", pvt->id);

				if (!pvt->initialized)
				{
					if (at_send_cvoice_test (pvt) || at_fifo_queue_add (pvt, CMD_AT_CVOICE, RES_OK))
					{
						ast_debug (1, "[%s] Error checking voice capabilities\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CVOICE:
				ast_debug (1, "[%s] Datacard has voice support\n", pvt->id);

				pvt->has_voice = 1;

				if (!pvt->initialized)
				{
					if (at_send_clip (pvt, 1) || at_fifo_queue_add (pvt, CMD_AT_CLIP, RES_OK))
					{
						ast_debug (1, "[%s] Error enabling calling line notification\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CLIP:
				ast_debug (1, "[%s] Calling line indication enabled\n", pvt->id);

				if (!pvt->initialized)
				{
					if (at_send_cssn (pvt, 1, 1) || at_fifo_queue_add (pvt, CMD_AT_CSSN, RES_OK))
					{
						ast_debug (1, "[%s] Error activating Supplementary Service Notification\n", pvt->id);
						goto e_return;
					}
				}

				break;

			case CMD_AT_CSSN:
				ast_debug (1, "[%s] Supplementary Service Notification enabled successful\n", pvt->id);

				if (!pvt->initialized)
				{
					/* set the SMS operating mode to text mode */
					if (at_send_cmgf (pvt, 1) || at_fifo_queue_add (pvt, CMD_AT_CMGF, RES_OK))
					{
						ast_debug (1, "[%s] Error setting CMGF\n", pvt->id);
						goto e_return;
					}
				}

				break;

			case CMD_AT_CMGF:
				ast_debug (1, "[%s] SMS text mode enabled\n", pvt->id);

				if (!pvt->initialized)
				{
					/* set text encoding to UCS-2 */
					if (at_send_cscs (pvt, "UCS2") || at_fifo_queue_add (pvt, CMD_AT_CSCS, RES_OK))
					{
						ast_debug (1, "[%s] Error setting CSCS (text encoding)\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CSCS:
				ast_debug (1, "[%s] UCS-2 text encoding enabled\n", pvt->id);

				pvt->use_ucs2_encoding = 1;

				if (!pvt->initialized)
				{
					/* set SMS storage location */
					if (at_send_cpms (pvt) || at_fifo_queue_add (pvt, CMD_AT_CPMS, RES_OK))
					{
						ast_debug (1, "[%s] Error setting CPMS\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CPMS:
				ast_debug (1, "[%s] SMS storage location is established\n", pvt->id);

				if (!pvt->initialized)
				{
					/* turn on SMS new message indication */
					if (at_send_cnmi (pvt) || at_fifo_queue_add (pvt, CMD_AT_CNMI, RES_OK))
					{
						ast_debug (1, "[%s] Error sending CNMI\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CNMI:
				ast_debug (1, "[%s] SMS new message indication enabled\n", pvt->id);
				ast_debug (1, "[%s] Datacard has sms support\n", pvt->id);

				pvt->has_sms = 1;

				if (!pvt->initialized)
				{
					if (at_send_csq (pvt) || at_fifo_queue_add (pvt, CMD_AT_CSQ, RES_OK))
					{
						ast_debug (1, "[%s] Error querying signal strength\n", pvt->id);
						goto e_return;
					}

					pvt->timeout = 10000;
					pvt->initialized = 1;
					ast_verb (3, "Datacard %s initialized and ready\n", pvt->id);
				}
				break;

			/* end initilization stuff */

			case CMD_AT_A:
				ast_debug(1, "[%s] Answer sent successfully\n", pvt->id);

				pvt->hookstate = 1;

				if (at_send_ddsetex (pvt) || at_fifo_queue_add (pvt, CMD_AT_DDSETEX, RES_OK))
				{
					ast_debug (1, "[%s] error sending AT^DDSETEX\n", pvt->id);
					goto e_return;
				}
				break;

			case CMD_AT_D:
				ast_debug (1, "[%s] Dial sent successfully\n", pvt->id);

				if (at_send_ddsetex (pvt) || at_fifo_queue_add (pvt, CMD_AT_DDSETEX, RES_OK))
				{
					ast_debug (1, "[%s] Error sending AT^DDSETEX\n", pvt->id);
					goto e_return;
				}
				break;

			case CMD_AT_DDSETEX:
				ast_debug (1, "[%s] AT^DDSETEX sent successfully\n", pvt->id);
				break;

			case CMD_AT_CHUP:
				ast_debug (1, "[%s] Successful hangup\n", pvt->id);
				break;

			case CMD_AT_CMGS:
				ast_debug (1, "[%s] Successfully sent sms message\n", pvt->id);
				pvt->outgoing_sms = 0;
				break;

			case CMD_AT_DTMF:
				ast_debug (1, "[%s] DTMF sent successfully\n", pvt->id);
				break;

			case CMD_AT_CUSD:
				ast_debug (1, "[%s] CUSD code sent successfully\n", pvt->id);
				break;

			case CMD_AT_COPS:
				ast_debug (1, "[%s] Provider query successfully\n", pvt->id);
				break;

			case CMD_AT_CMGD:
				ast_debug (1, "[%s] SMS message deleted successfully\n", pvt->id);
				break;

			case CMD_AT_CSQ:
				ast_debug (1, "[%s] Got signal strength result\n", pvt->id);
				break;

			case CMD_AT_CLVL:
				if (pvt->volume_synchronized == 0)
				{
					pvt->volume_synchronized = 1;

					if (at_send_clvl (pvt, 5) || at_fifo_queue_add (pvt, CMD_AT_CLVL, RES_OK))
					{
						ast_debug (1, "[%s] Error syncronizing audio level (part 2/2)\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_UNKNOWN:
			default:
				ast_debug (1, "[%s] Recieved 'OK' for unhandled command '%s'\n", pvt->id, at_cmd2str (e->cmd));
				break;
		}

		at_fifo_queue_rem (pvt);
	}
	else if (e)
	{
		ast_debug (1, "[%s] Recieved 'OK' when expecting '%s', ignoring\n", pvt->id, at_res2str (e->res));
	}
	else
	{
		ast_debug (1, "[%s] Recieved unexpected 'OK'\n", pvt->id);
	}

	return 0;

e_return:
	at_fifo_queue_rem (pvt);

	return -1;
}

/*!
 * \brief Handle ERROR response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_error (pvt_t* pvt)
{
	at_queue_t* e;

	if ((e = at_fifo_queue_head (pvt)) && (e->res == RES_OK || e->res == RES_ERROR ||
			e->res == RES_CMS_ERROR || e->res == RES_CMGR || e->res == RES_SMS_PROMPT))
	{
		switch (e->cmd)
		{
        		/* initilization stuff */
			case CMD_AT:
			case CMD_AT_Z:
			case CMD_AT_E:
			case CMD_AT_U2DIAG:
				ast_debug (1, "[%s] Error '%s' failed\n", pvt->id, at_cmd2str (e->cmd));
				goto e_return;

			case CMD_AT_CGMI:
				ast_debug (1, "[%s] Getting manufacturer info failed\n", pvt->id);
				goto e_return;

			case CMD_AT_CGMM:
				ast_debug(1, "[%s] Getting model info failed\n", pvt->id);
				goto e_return;

			case CMD_AT_CGMR:
				ast_debug(1, "[%s] Getting firmware info failed\n", pvt->id);
				goto e_return;

			case CMD_AT_CGSN:
				ast_debug (1, "[%s] Getting IMEI number failed\n", pvt->id);
				goto e_return;

			case CMD_AT_CPIN:
				ast_debug(1, "[%s] Error checking PIN state\n", pvt->id);
				goto e_return;

			case CMD_AT_COPS_INIT:
				ast_debug (1, "[%s] Error setting operator select parameters\n", pvt->id);
				goto e_return;

			case CMD_AT_CREG_INIT:
				ast_debug(1, "[%s] Error enableling registration info\n", pvt->id);
				goto e_return;

			case CMD_AT_CREG:
				ast_debug (1, "[%s] Error getting registration info\n", pvt->id);

				if (!pvt->initialized)
				{
					/* this is not a fatal error, let's continue with initilization */
					if (at_send_cnum (pvt) || at_fifo_queue_add (pvt, CMD_AT_CNUM, RES_OK))
					{
						ast_debug (1, "[%s] Error checking subscriber phone number\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CNUM:
				ast_debug (1, "[%s] Error checking subscriber phone number\n", pvt->id);
				ast_verb (3, "Datacard %s needs to be reinitialized. The SIM card is not ready yet\n", pvt->id);
				goto e_return;

			case CMD_AT_CVOICE:
				ast_debug (1, "[%s] Datacard has NO voice support\n", pvt->id);

				pvt->has_voice = 0;

				if (!pvt->initialized)
				{
					/* this is not a fatal error, let's continue with initilization */
					if (at_send_cmgf (pvt, 1) || at_fifo_queue_add (pvt, CMD_AT_CMGF, RES_OK))
					{
						ast_debug (1, "[%s] Error setting CMGF\n", pvt->id);
						goto e_return;
					}
				}
				break;

			case CMD_AT_CLIP:
				ast_debug (1, "[%s] Error enabling calling line indication\n", pvt->id);
				goto e_return;

			case CMD_AT_CSSN:
				ast_debug (1, "[%s] Error Supplementary Service Notification activation failed\n", pvt->id);
				goto e_return;

			case CMD_AT_CMGF:
			case CMD_AT_CPMS:
			case CMD_AT_CNMI:
				ast_debug (1, "[%s] Error '%s' failed\n", pvt->id, at_cmd2str (e->cmd));
				ast_debug (1, "[%s] No SMS support\n", pvt->id);

				pvt->has_sms = 0;

				if (!pvt->initialized)
				{
					if (pvt->has_voice)
					{
						if (at_send_csq (pvt) || at_fifo_queue_add (pvt, CMD_AT_CSQ, RES_OK))
						{
							ast_debug (1, "[%s] Error querying signal strength\n", pvt->id);
							goto e_return;
						}

						pvt->timeout = 10000;
						pvt->initialized = 1;
						ast_verb (3, "Datacard %s initialized and ready\n", pvt->id);
					}

					goto e_return;
				}
				break;

			case CMD_AT_CSCS:
				/* this is not a fatal error, let's continue with initilization */
				ast_debug (1, "[%s] No UCS-2 encoding support\n", pvt->id);

				pvt->use_ucs2_encoding = 0;

				if (!pvt->initialized)
				{
					/* set SMS storage location */
					if (at_send_cpms (pvt) || at_fifo_queue_add (pvt, CMD_AT_CPMS, RES_OK))
					{
						ast_debug(1, "[%s] error setting CPMS\n", pvt->id);
						goto e_return;
					}
				}
				break;

			/* end initilization stuff */

			case CMD_AT_A:
				ast_debug (1, "[%s] Answer failed\n", pvt->id);
				channel_queue_hangup (pvt);
				break;

			case CMD_AT_D:
				ast_debug(1, "[%s] Dial failed\n", pvt->id);
				pvt->hookstate = 0;
				channel_queue_control (pvt, AST_CONTROL_CONGESTION);
				break;

			case CMD_AT_DDSETEX:
				ast_debug (1, "[%s] AT^DDSETEX failed\n", pvt->id);
				break;

			case CMD_AT_CHUP:
				ast_debug (1, "[%s] Error sending hangup, disconnecting\n", pvt->id);
				goto e_return;

			case CMD_AT_CMGR:
				ast_debug (1, "[%s] Error reading SMS message\n", pvt->id);
				pvt->incoming_sms = 0;
				break;

			case CMD_AT_CMGD:
				ast_debug (1, "[%s] Error deleting SMS message\n", pvt->id);
				pvt->incoming_sms = 0;
				break;

			case CMD_AT_CMGS:
				ast_debug (1, "[%s] Error sending SMS message\n", pvt->id);
				pvt->outgoing_sms = 0;
				break;

			case CMD_AT_DTMF:
				ast_debug (1, "[%s] Error sending DTMF\n", pvt->id);
				break;

			case CMD_AT_COPS:
				ast_debug (1, "[%s] Could not get provider name\n", pvt->id);
				break;

			case CMD_AT_CLVL:
				ast_debug (1, "[%s] Error syncronizing audio level\n", pvt->id);

				/* this is not a fatal error, let's continue with initilization */
				pvt->volume_synchronized = 0;
				break;

			case CMD_UNKNOWN:
			default:
				ast_debug (1, "[%s] Recieved 'ERROR' for unhandled command '%s'\n", pvt->id, at_cmd2str (e->cmd));
				break;
		}

		at_fifo_queue_rem (pvt);
	}
	else if (e)
	{
		ast_debug (1, "[%s] Recieved 'ERROR' when expecting '%s', ignoring\n", pvt->id, at_res2str (e->res));
	}
	else
	{
		ast_debug (1, "[%s] Recieved unexpected 'ERROR'\n", pvt->id);
	}

	return 0;

e_return:
	at_fifo_queue_rem (pvt);

	return -1;
}

/*!
 * \brief Handle ^RSSI response Here we get the signal strength.
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_rssi (pvt_t* pvt, char* str, size_t len)
{
	if ((pvt->rssi = at_parse_rssi (pvt, str, len)) == -1)
	{
		return -1;
	}

	return 0;
}

/*!
 * \brief Handle ^MODE response Here we get the link mode (GSM, UMTS, EDGE...).
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_mode (pvt_t* pvt, char* str, size_t len)
{
	return at_parse_mode (pvt, str, len, &pvt->linkmode, &pvt->linksubmode);
}

/*!
 * \brief Handle ^ORIG response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_orig (pvt_t* pvt, char* str, size_t len)
{
	int	call_index = 1;
	int	call_type  = 0;

	channel_queue_control (pvt, AST_CONTROL_PROGRESS);

	/*
	 * parse ORIG info in the following format:
	 * ^ORIG:<call_index>,<call_type>
	 */

	if (!sscanf (str, "^ORIG:%d,%d", &call_index, &call_type))
	{
		ast_debug (1, "[%s] Error parsing ORIG event '%s'\n", pvt->id, str);
		return -1;
	}

	ast_debug (1, "[%s] Recieved call_index: %d\n", pvt->id, call_index);
	ast_debug (1, "[%s] Recieved call_type:  %d\n", pvt->id, call_type);

	if (at_send_clvl (pvt, 1) || at_fifo_queue_add (pvt, CMD_AT_CLVL, RES_OK))
	{
		ast_debug (1, "[%s] Error syncronizing audio level (part 1/2)\n", pvt->id);
	}

	return 0;
}

/*!
 * \brief Handle ^CEND response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cend (pvt_t* pvt, char* str, size_t len)
{
	int	call_index = 0;
	int	duration   = 0;
	int	end_status = 0;
	int	cc_cause   = 0;

	/*
	 * parse CEND info in the following format:
	 * ^CEND:<call_index>,<duration>,<end_status>[,<cc_cause>]
	 */

	if (!sscanf (str, "^CEND:%d,%d,%d,%d", &call_index, &duration, &end_status, &cc_cause))
	{
		ast_debug (1, "[%s] Could not parse all CEND parameters\n", pvt->id);
	}

	ast_debug (1, "[%s] CEND: call_index: %d\n",	pvt->id, call_index);
	ast_debug (1, "[%s] CEND: duration:   %d\n",	pvt->id, duration);
	ast_debug (1, "[%s] CEND: end_status: %d\n",	pvt->id, end_status);
	ast_debug (1, "[%s] CEND: cc_cause:   %d\n",	pvt->id, cc_cause);

	pvt->hangupcause = cc_cause;

	ast_debug (1, "[%s] Line disconnected\n", pvt->id);

	if (pvt->owner)
	{
		ast_debug (1, "[%s] hanging up owner\n", pvt->id);

		if (channel_queue_hangup (pvt))
		{
			ast_log (LOG_ERROR, "[%s] Error queueing hangup, disconnecting...\n", pvt->id);
			return -1;
		}
	}

	pvt->hookstate = 0;
	pvt->needring  = 0;
	pvt->incoming  = 0;
	pvt->outgoing  = 0;
	pvt->volume_synchronized = 0;

	return 0;
}

/*!
 * \brief Handle ^CONN response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_conn (pvt_t* pvt)
{
	if (pvt->outgoing)
	{
		ast_debug (1, "[%s] Remote end answered\n", pvt->id);
		channel_queue_control (pvt, AST_CONTROL_ANSWER);
	}
	else if (pvt->incoming && pvt->answered)
	{
		ast_setstate (pvt->owner, AST_STATE_UP);
	}

	return 0;
}

/*!
 * \brief Handle +CLIP response
 * \param pvt -- pvt structure
 * \param str -- null terminated strfer containing response
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_clip (pvt_t* pvt, char* str, size_t len)
{
	struct ast_channel*	channel;
	char*			clip;

	ast_debug (1, "[%s] Executing at_response_clip\n", pvt->id);

	if ((clip = at_parse_clip (pvt, str, len)) == NULL)
	{
		ast_debug (1, "[%s] Error parsing CLIP: %s\n", pvt->id, str);
	}
	
	if (pvt->needring == 0)
	{
		pvt->incoming = 1;
		
		if (!(channel = channel_new (pvt, AST_STATE_RING, clip)))
		{
			ast_log (LOG_ERROR, "[%s] Unable to allocate channel for incoming call\n", pvt->id);
			if (at_send_chup (pvt) || at_fifo_queue_add (pvt, CMD_AT_CHUP, RES_OK))
			{
				ast_log (LOG_ERROR, "[%s] Error sending AT+CHUP command\n", pvt->id);
			}

			return -1;
		}

		/* from this point on, we need to send a chup in the event of a hangup */
		pvt->hookstate = 1;
		/* We dont need to send ring a 2nd time */
		pvt->needring = 1;

		if (ast_pbx_start (channel))
		{
			ast_log (LOG_ERROR, "[%s] Unable to start pbx on incoming call\n", pvt->id);
			channel_ast_hangup (pvt);

			return -1;
		}
	}

	return 0;
}

/*!
 * \brief Handle RING response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_ring (pvt_t* pvt)
{
	/* We only want to syncronize volume on the first ring */
	if (!pvt->incoming)
	{
		if (at_send_clvl (pvt, 1) || at_fifo_queue_add (pvt, CMD_AT_CLVL, RES_OK))
		{
			ast_debug (1, "[%s] Error syncronizing audio level (part 1/2)\n", pvt->id);
		}
	}

	pvt->incoming = 1;

	return 0;
}

/*!
 * \brief Handle +CMTI response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cmti (pvt_t* pvt, char* str, size_t len)
{
	int index = at_parse_cmti (pvt, str, len);

	if (index > -1)
	{
		ast_debug (1, "[%s] Incoming SMS message\n", pvt->id);

		pvt->sms_storage_position = index;
		if (at_send_cmgr (pvt, index) || at_fifo_queue_add (pvt, CMD_AT_CMGR, RES_CMGR))
		{
			ast_debug (1, "[%s] Error sending CMGR to retrieve SMS message\n", pvt->id);
			return -1;
		}

		pvt->incoming_sms = 1;

		return 0;
	}
	else
	{
		ast_debug (1, "[%s] Error parsing incoming sms message alert, disconnecting\n", pvt->id);
		return -1;
	}
}

/*!
 * \brief Handle +CMGR response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cmgr (pvt_t* pvt, char* str, size_t len)
{
	at_queue_t*		e;
	ssize_t			res;
	char			sms_utf8_str[4096];
	char			from_number_utf8_str[1024];
	char*			from_number = NULL;
	char*			text = NULL;
	struct ast_channel*	channel;

	if ((e = at_fifo_queue_head (pvt)) && e->res == RES_CMGR)
	{
		at_fifo_queue_rem (pvt);

		if (at_parse_cmgr (pvt, str, len, &from_number, &text))
		{
			ast_debug (1, "[%s] Error parsing SMS message, disconnecting\n", pvt->id);
			return -1;
		}

		ast_debug (1, "[%s] Successfully read SMS message\n", pvt->id);

		pvt->incoming_sms = 0;

		if (pvt->use_ucs2_encoding)
		{
			res = hexstr_ucs2_to_utf8 (text, strlen (text), sms_utf8_str, sizeof (sms_utf8_str));
			if (res > 0)
			{
				text = sms_utf8_str;
			}
			else
			{
				ast_log (LOG_ERROR, "[%s] Error parsing SMS (convert UCS-2 to UTF-8): %s\n", pvt->id, text);
			}

			res = hexstr_ucs2_to_utf8 (from_number, strlen (from_number), from_number_utf8_str, sizeof (from_number_utf8_str));
			if (res > 0)
			{
				from_number = from_number_utf8_str;
			}
			else
			{
				ast_log (LOG_ERROR, "[%s] Error parsing SMS from_number (convert UCS-2 to UTF-8): %s\n", pvt->id, from_number);
			}
		}

		ast_verb (1, "[%s] Got SMS from %s: %s\n", pvt->id, from_number, text);

#ifdef __MANAGER__
		manager_event_new_sms (pvt, from_number, text);
#endif

	}
	else if (e)
	{
		ast_debug (1, "[%s] Recieved '+CMGR' when expecting '%s' response to '%s', ignoring\n", pvt->id,
				at_res2str (e->res), at_cmd2str (e->cmd));
	}
	else
	{
		ast_debug (1, "[%s] Recieved unexpected '+CMGR'\n", pvt->id);
	}

	if (pvt->auto_delete_sms)
	{
		if (at_send_cmgd (pvt, pvt->sms_storage_position) || at_fifo_queue_add (pvt, CMD_AT_CMGD, RES_OK))
		{
			ast_debug (1, "[%s] Error sending CMGD to delete SMS message\n", pvt->id);
			return -1;
		}
	}

	return 0;
}

/*!
 * \brief Send an SMS message from the queue.
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_sms_prompt (pvt_t* pvt)
{
	at_queue_t* e;

	if ((e = at_fifo_queue_head (pvt)) && e->res == RES_SMS_PROMPT)
	{
		at_fifo_queue_rem (pvt);

		if (!e->data && (at_send_sms_text (pvt, e->data) || at_fifo_queue_add (pvt, CMD_AT_CMGS, RES_OK)))
		{
			ast_debug (1, "[%s] error sending sms message\n", pvt->id);
			return -1;
		}
	}
	else if (e)
	{
		ast_debug (1, "[%s] Recieved sms prompt when expecting '%s' response to '%s', ignoring\n", pvt->id,
				at_res2str (e->res), at_cmd2str (e->cmd));
	}
	else
	{
		ast_debug (1, "[%s] Recieved unexpected sms prompt\n", pvt->id);
	}

	return 0;
}

/*!
 * \brief Handle CUSD response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cusd (pvt_t* pvt, char* str, size_t len)
{
	int	res;
	char*	cusd;
	char	cusd_utf8_str[4096];

	if (!(cusd = at_parse_cusd (pvt, str, len)))
	{
		ast_verb (1, "[%s] error parsing CUSD: '%.*s'\n", pvt->id, (int) len, str);
		return 0;
	}

	ast_debug (1, "[%s] Got CUSD response: '%s'\n", pvt->id, cusd);

	if (pvt->use_ucs2_encoding)
	{
		res = hexstr_ucs2_to_utf8 (cusd, strlen (cusd), cusd_utf8_str, sizeof (cusd_utf8_str));
		if (res > 0)
		{
			cusd = cusd_utf8_str;
		}
		else
		{
			ast_log (LOG_ERROR, "[%s] error parsing CUSD (convert UCS-2 to UTF-8): %s\n", pvt->id, cusd);
		}
	}

	ast_verb (1, "[%s] Got CUSD response: %s\n", pvt->id, cusd);

#ifdef __MANAGER__
	manager_event_new_cusd (pvt, cusd);
#endif

	return 0;
}

/*!
 * \brief Handle BUSY response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_busy (pvt_t* pvt)
{
	pvt->hookstate = 1;
	pvt->hangupcause = AST_CAUSE_USER_BUSY;
	channel_queue_control (pvt, AST_CONTROL_BUSY);

	return 0;
}
 
/*!
 * \brief Handle NO DIALTONE response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_no_dialtone (pvt_t* pvt)
{
	ast_verb (1, "[%s] Datacard reports 'NO DIALTONE'\n", pvt->id);

	pvt->hookstate = 1;
	channel_queue_control (pvt, AST_CONTROL_CONGESTION);

	return 0;
}

/*!
 * \brief Handle NO CARRIER response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_no_carrier (pvt_t* pvt)
{
	ast_verb (1, "[%s] Datacard reports 'NO CARRIER'\n", pvt->id);

	pvt->hookstate = 1;
	channel_queue_control (pvt, AST_CONTROL_CONGESTION);

	return 0;
}

/*!
 * \brief Handle +CPIN response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cpin (pvt_t* pvt, char* str, size_t len)
{
	return at_parse_cpin (pvt, str, len);
}

/*!
 * \brief Handle ^SMMEMFULL response This event notifies us, that the sms storage is full.
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_smmemfull (pvt_t* pvt)
{
	ast_log (LOG_ERROR, "[%s] SMS storage is full\n", pvt->id);
	return 0;
}

/*!
 * \brief Handle +CSQ response Here we get the signal strength and bit error rate.
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */
static inline int at_response_csq (pvt_t* pvt, char* str, size_t len)
{
	return at_parse_csq (pvt, str, len, &pvt->rssi, &pvt->ber);
}

/*!
 * \brief Handle +CNUM response Here we get our own phone number.
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cnum (pvt_t* pvt, char* str, size_t len)
{
	char* number = at_parse_cnum (pvt, str, len);

	if (number)
	{
		ast_copy_string (pvt->number, number, sizeof (pvt->number));
		return 0;
	}

	ast_copy_string (pvt->number, "Unknown", sizeof (pvt->number));

	return -1;
}

/*!
 * \brief Handle +COPS response Here we get the GSM provider name.
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cops (pvt_t* pvt, char* str, size_t len)
{
	char* provider_name = at_parse_cops (pvt, str, len);

	if (provider_name)
	{
		ast_copy_string (pvt->provider_name, provider_name, sizeof (pvt->provider_name));
		return 0;
	}

	ast_copy_string (pvt->provider_name, "NONE", sizeof (pvt->provider_name));

	return -1;
}

/*!
 * \brief Handle +CREG response Here we get the GSM registration status.
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_creg (pvt_t* pvt)
{
	if (at_send_cops (pvt) || at_fifo_queue_add (pvt, CMD_AT_COPS, RES_OK))
	{
		ast_debug (1, "[%s] Error sending query for provider name\n", pvt->id);
	}

	return 0;
}

/*!
 * \brief Handle AT+CGMI response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cgmi (pvt_t* pvt, char* str, size_t len)
{
	ast_copy_string (pvt->manufacturer, str, sizeof (pvt->manufacturer));

	return 0;
}

/*!
 * \brief Handle AT+CGMM response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cgmm (pvt_t* pvt, char* str, size_t len)
{
	ast_copy_string (pvt->model, str, sizeof (pvt->model));

	return 0;
}

/*!
 * \brief Handle AT+CGMR response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cgmr (pvt_t* pvt, char* str, size_t len)
{
	ast_copy_string (pvt->firmware, str, sizeof (pvt->firmware));

	return 0;
}

/*!
 * \brief Handle AT+CGSN response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_response_cgsn (pvt_t* pvt, char* str, size_t len)
{
	ast_copy_string (pvt->imei, str, sizeof (pvt->imei));

	return 0;
}