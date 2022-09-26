#include "eventtype.h"

static EErrorReason  ERR[] =
{
	{ERRC_SUCCESS , "Ok."},
	{ERRC_INVALID_MSG,"Invalid message."},
	{ERRC_INVAID_DATA, "Invalid data."},
	{ERRC_MTTHOD_NOT_ALLOWED,"Method not allowed"},
	{ERRO_PROCESS_FAILED, "Process is failed"},
	{ERRO_BIKE_IS_TOOK,"Bike is took"},
	{ERRO_BIKE_IS_RUNNING,"Bike is running"},
	{ERRO_BIKE_IS_DAMAGED,"Bike is damaged"},
	{ERR_NULL,"Undefined"}
};

const char* getReasonByErrorCode(i32  code)
{
	i32 i = 0;
	for (i = 0; ERR[i].code != ERR_NULL; i++)
	{
		if (ERR[i].code == code)
		{
			return ERR[i].reason;
		}
	}
	return ERR[i].reason;    // "Undefined"
}

