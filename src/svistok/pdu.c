/* Svistok-only composition fragment. */

EXPORT_DEF const char * pdu_parse_cds(char ** pdu, size_t tpdu_length, char * oa, size_t oa_len, str_encoding_t * oa_enc, char ** msg, str_encoding_t * msg_enc)
{
	const char * err = NULL;
	size_t pdu_length = strlen(*pdu);

	/* decode SCA */
	int field_len = pdu_parse_sca(pdu, &pdu_length);
	if(field_len > 0)
	{
	    //if(tpdu_length * 2 == pdu_length)
	    //{
		int pdu_type = pdu_parse_byte(pdu, &pdu_length);
		if(pdu_type >= 0)
		{
			/* TODO: also handle PDUTYPE_MTI_SMS_SUBMIT_REPORT and PDUTYPE_MTI_SMS_STATUS_REPORT */
			if(PDUTYPE_MTI(pdu_type) == PDUTYPE_MTI_SMS_STATUS_REPORT)
			{
				int something = pdu_parse_byte(pdu, &pdu_length);

				int oa_digits = pdu_parse_byte(pdu, &pdu_length);
				if(oa_digits > 0)
				{
					int oa_toa;
					field_len = pdu_parse_number(pdu, &pdu_length, oa_digits, &oa_toa, oa, oa_len);
					*oa_enc = STR_ENCODING_7BIT;

					/*if(field_len <= 0 ) {*msg=*pdu; return NULL;}*/
					*msg=*pdu;
					**msg=0;
					return NULL;

					/*
					if(field_len > 0)
					{
						int pid = pdu_parse_byte(pdu, &pdu_length);
						*oa_enc = STR_ENCODING_7BIT;
						if(pid >= 0)
						{
						   // TODO: support other types of messages
						   if(pid == PDU_PID_SMS)
						   {
							int dcs = pdu_parse_byte(pdu, &pdu_length);
							if(dcs >= 0)
							{
							    // TODO: support compression
							    if( PDU_DCS_76(dcs) == PDU_DCS_76_00
							    		&&
							    	PDU_DCS_COMPRESSION(dcs) == PDU_DCS_NOT_COMPESSED
							    		&&
							    		(
							    		PDU_DCS_ALPABET(dcs) == PDU_DCS_ALPABET_7BIT
							    			||
							    		PDU_DCS_ALPABET(dcs) == PDU_DCS_ALPABET_8BIT
							    			||
							    		PDU_DCS_ALPABET(dcs) == PDU_DCS_ALPABET_UCS2
							    		)
							    	)
							    {
								int ts = pdu_parse_timestamp(pdu, &pdu_length);
								*msg_enc = pdu_dcs_alpabet2encoding(PDU_DCS_ALPABET(dcs));
								if(ts >= 0)
								{
									int udl = pdu_parse_byte(pdu, &pdu_length);
									if(udl >= 0)
									{
										// calculate number of octets in UD 
										if(PDU_DCS_ALPABET(dcs) == PDU_DCS_ALPABET_7BIT)
											udl = ((udl + 1) * 7) >> 3;
										if((size_t)udl * 2 == pdu_length)
										{
											if(PDUTYPE_UDHI(pdu_type) == PDUTYPE_UDHI_HAS_HEADER)
											{
												// TODO: implement header parse
												int udhl = pdu_parse_byte(pdu, &pdu_length);
												if(udhl >= 0)
												{
													// NOTE: UDHL count octets no need calculation 
													if(pdu_length >= (size_t)(udhl * 2))
													{
														// skip UDH
														*pdu += udhl * 2;
														pdu_length -= udhl * 2;
													}
													else
													{
														err = "Invalid UDH";
													}
												}
												else
												{
													err = "Can't parse UDHL";
												}
											}
											// save message 
											*msg = *pdu;
										}
										else
										{
											*pdu -= 2;
											err = "UDL not match with UD length";
										}
									}
									else
									{
										err = "Can't parse UDL";
									}
								}
								else
								{
									err = "Can't parse Timestamp";
								}
							    }
							    else
							    {
								*pdu -= 2;
								err = "Unsupported DCS value";
							    }
							}
							else
							{
								err = "Can't parse DSC";
							}
						    }
						    else
						    {
						    	err = "Unhandled PID value, only SMS supported";
						    }
						}
						else
						{
							err = "Can't parse PID";
						}
					}
					else
					{
						err = "Can't parse OA";
					}*/

				}
				else
				{
					err = "Can't parse length of OA";
				}
			}
			else
			{
				*pdu -= 2;
				err = "Unhandled PDU Type MTI only SMS-DELIVER supported";
			}
		}
		else
		{
			err = "Can't parse PDU Type";
		}
	//    }
	//    else
	//    {
	//	err = "TPDU length not matched with actual length";
	    //}
	}
	else
	{
		err = "Can't parse SCA";
	}

	return err;
}
