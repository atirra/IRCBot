#include "DatabaseWrapper.h"

const std::string CDatabaseWrapper::LogTableName = std::string("chatlog");
const std::string CDatabaseWrapper::LastSeenTableName = std::string("lastseen");
const std::string CDatabaseWrapper::PrepareQuery = std::string("CREATE TABLE IF NOT EXISTS "+CDatabaseWrapper::LogTableName+" (Time INTEGER, Channel TEXT, Message TEXT); CREATE TABLE IF NOT EXISTS "+CDatabaseWrapper::LastSeenTableName+" (Time INTEGER, Channel TEXT, Nick TEXT, Reason TEXT, PRIMARY KEY (Nick, Channel));");
const std::string CDatabaseWrapper::InsertLogQuery = std::string("INSERT INTO "+CDatabaseWrapper::LogTableName+" VALUES (strftime('%s','now'), ?, ?);");
const std::string CDatabaseWrapper::GetLogQuery = std::string("SELCET strftime('%d.%m.%Y %M:%H', Time, 'unixepoch') AS TimeStamp, Message FROM "+CDatabaseWrapper::LogTableName+" WHERE Channel = ? ORDER BY Time ASC;");
const std::string CDatabaseWrapper::InsertLastSeenQuery = std::string("INSERT OR REPLACE INTO "+CDatabaseWrapper::LastSeenTableName+" VALUES(strftime('%s','now'), ?, ?, ?);");
const std::string CDatabaseWrapper::GetLastSeenQuery = std::string("SELECT strftime('%d.%m.%Y %M:%H', Time, 'unixepoch') AS TimeStamp, Reason FROM "+CDatabaseWrapper::LastSeenTableName+" WHERE (Channel = ?) AND (Nick = ?) ORDER BY Time ASC;");

CDatabaseWrapper CDatabaseWrapper::Singleton = CDatabaseWrapper();

//------------------------------------------//
//											//
//		  Konstructor & Destructor			//
//											//
//------------------------------------------//

CDatabaseWrapper::CDatabaseWrapper()
	:m_dbHandle(NULL)
	,m_InsertLogStatement(NULL)
	,m_GetLogStatement(NULL)
	,m_InsertLastSeenStatement(NULL)
	,m_GetLastSeenStatement(NULL)
{
	OpenDB();
	PrepareDatabase();
	PrepareStatements();
}

CDatabaseWrapper::~CDatabaseWrapper()
{
	CloseDB();
}

//------------------------------------------//
//											//
//		 		Open & Prepare				//
//											//
//------------------------------------------//

void CDatabaseWrapper::OpenDB()
{
	int Result = sqlite3_open("IRCLog.sqlite", &m_dbHandle);

	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "On opening the DB -> ", sqlite3_errmsg(m_dbHandle) });
		sqlite3_close(m_dbHandle);
		m_dbHandle = NULL;
	}
}

void CDatabaseWrapper::PrepareDatabase()
{
	if(!Connected())
	{
		return;
	}

	char * ErrorMsg;
	int Result = sqlite3_exec(m_dbHandle, CDatabaseWrapper::PrepareQuery.c_str(), NULL, NULL, &ErrorMsg);

	if(Result != 0)
	{
		Output::Error("SQLLite Plugin", { "On executin the initial statement -> ", ErrorMsg});
		sqlite3_free(ErrorMsg);
		CloseDB();
		return;
	}
}

void CDatabaseWrapper::PrepareStatements()
{	
	if(!Connected())
	{
		return;
	}

	int Result = 0;

	//Insert Log
	Result = sqlite3_prepare_v2(m_dbHandle, CDatabaseWrapper::InsertLogQuery.c_str(), CDatabaseWrapper::InsertLogQuery.size(), &m_InsertLogStatement, NULL);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "On preparing the InsertLog Statement -> ", sqlite3_errmsg(m_dbHandle) });
		CloseDB();
		return;
	}

	//Get Log
	/*Result = sqlite3_prepare_v2(m_dbHandle, CDatabaseWrapper::GetLogQuery.c_str(), CDatabaseWrapper::GetLogQuery.size(), &m_GetLogStatement, NULL);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "On preparing the GetLog Statement -> ", sqlite3_errmsg(m_dbHandle) });
		CloseDB();
		return;
	}
	*/
	//Insert LastSeen
	Result = sqlite3_prepare_v2(m_dbHandle, CDatabaseWrapper::InsertLastSeenQuery.c_str(), CDatabaseWrapper::InsertLastSeenQuery.size(), &m_InsertLastSeenStatement, NULL);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "On preparing the InsertLastSeen Statement -> ", sqlite3_errmsg(m_dbHandle) });
		CloseDB();
		return;
	}

	//Get LastSeen
	/*Result = sqlite3_prepare_v2(m_dbHandle, CDatabaseWrapper::GetLastSeenQuery.c_str(), CDatabaseWrapper::GetLastSeenQuery.size(), &m_GetLastSeenStatement, NULL);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "On preparing the GetLastSeen Statement -> ", sqlite3_errmsg(m_dbHandle) });
		CloseDB();
		return;
	}
	*/
}

//------------------------------------------//
//											//
//		 	Finalize & Close				//
//											//
//------------------------------------------//


void CDatabaseWrapper::CloseDB()
{
	FinalizeStatements();

	int Result = sqlite3_close(m_dbHandle);
	m_dbHandle = NULL;

	if(Result != SQLITE_OK)
	{
		//Error
		Output::Error("SQLLite Plugin", { "On closing the DB -> ", sqlite3_errmsg(m_dbHandle) });
	}
}

void CDatabaseWrapper::FinalizeStatements()
{
	sqlite3_finalize(m_InsertLogStatement);
	sqlite3_finalize(m_GetLogStatement);
	sqlite3_finalize(m_InsertLastSeenStatement);
	sqlite3_finalize(m_GetLastSeenStatement);

	m_InsertLogStatement = NULL;
	m_GetLogStatement = NULL;
	m_InsertLastSeenStatement = NULL;
	m_GetLastSeenStatement = NULL;
}



//------------------------------------------//
//											//
//		 	Finalize & Close				//
//											//
//------------------------------------------//

void CDatabaseWrapper::LogMessage(const std::string & Channel, const std::string & Message)
{
	if(!Connected())
	{
		return;
	}

	if((Channel.size() <= 0) || (Message.size() <= 0))
	{
		return;
	}

	int Result = 0;
	sqlite3_reset(m_InsertLogStatement);
	sqlite3_clear_bindings(m_InsertLogStatement);

	Result = sqlite3_bind_text(m_InsertLogStatement, 1, Channel.c_str(), Channel.size(), SQLITE_STATIC);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "Bind Channel for Log Message -> ", sqlite3_errmsg(m_dbHandle) });
		return;
	}

	Result = sqlite3_bind_text(m_InsertLogStatement, 2, Message.c_str(), Message.size(), SQLITE_STATIC);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "Bind Message for Log Message -> ", sqlite3_errmsg(m_dbHandle) });
		return;
	}

	while((Result = sqlite3_step(m_InsertLogStatement))  != SQLITE_DONE)
	{
		if(Result != SQLITE_BUSY)
		{
			Output::Error("SQLLite Plugin", { "Executing the Insert Log -> ", sqlite3_errmsg(m_dbHandle) });
			return;
		}		
	}
}

void CDatabaseWrapper::GetLog(const std::string & Channel, StringPairVector & OutVector)
{

}


//------------------------------------------//
//											//
//		 	Finalize & Close				//
//											//
//------------------------------------------//

void CDatabaseWrapper::UserLeft(const std::string & Channel, const std::string & Nick, const std::string & Reason)
{
	if(!Connected())
	{
		return;
	}

	if((Channel.size() <= 0) || (Nick.size() <= 0))
	{
		return;
	}

	int Result = 0;
	sqlite3_reset(m_InsertLastSeenStatement);
	sqlite3_clear_bindings(m_InsertLastSeenStatement);

	Result = sqlite3_bind_text(m_InsertLastSeenStatement, 1, Channel.c_str(), Channel.size(), SQLITE_STATIC);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "Bind Channel for LastSeen -> ", sqlite3_errmsg(m_dbHandle) });
		return;
	}

	Result = sqlite3_bind_text(m_InsertLastSeenStatement, 2, Nick.c_str(), Nick.size(), SQLITE_STATIC);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "Bind Nick for LastSeen-> ", sqlite3_errmsg(m_dbHandle) });
		return;
	}

	Result = sqlite3_bind_text(m_InsertLastSeenStatement, 3, Reason.c_str(), Reason.size(), SQLITE_STATIC);
	if(Result != SQLITE_OK)
	{
		Output::Error("SQLLite Plugin", { "Bind Reason for LastSeen-> ", sqlite3_errmsg(m_dbHandle) });
		return;
	}

	while((Result = sqlite3_step(m_InsertLastSeenStatement))  != SQLITE_DONE)
	{
		if(Result != SQLITE_BUSY)
		{
			Output::Error("SQLLite Plugin", { "Executing the Insert Log -> ", sqlite3_errmsg(m_dbHandle) });
			return;
		}		
	}
}

std::string CDatabaseWrapper::LastSeen(const std::string & Nick)
{
	return std::string("");
}


