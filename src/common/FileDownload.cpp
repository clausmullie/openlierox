/////////////////////////////////////////
//
//   OpenLieroX
//
//   Auxiliary Software class library
//
//   based on the work of JasonB
//   enhanced by Dark Charlie and Albert Zeyer
//
//   code under LGPL
//
/////////////////////////////////////////


// File downloading over HTTP
// Created 13/10/07
// By Karel Petranek, Albert Zeyer and Martin Griffin

#ifdef _MSC_VER
#pragma warning(disable: 4786)  // WARNING: identifier XXX was truncated to 255 characters in the debug info
#pragma warning(disable: 4503)  // WARNING: decorated name length exceeded, name was truncated
#endif

#include "StringUtils.h"
#include "FindFile.h"
#include "FileDownload.h"

//
// Download error strings
//
const std::string sDownloadErrors[] =  {
	"No error",
	"No file specified",
	"No destination specified",
	"No servers available",
	"HTTP error",
	"Error while saving"
};

//
// Main thread function
//
int DownloadThreadMain(void *param)
{
	// Check
	if (param == NULL)
		return -1;

	CFileDownloader *owner = (CFileDownloader *)param;
	owner->Lock();
	std::list<CFileDownload> *downloads = owner->GetDownloads();
	owner->Unlock();

	bool downloading = false;
	while (!owner->ShouldBreakThread())  {
		downloading = false;

		owner->Lock();

		// Process the downloads
		for (std::list<CFileDownload>::iterator i = downloads->begin(); i != downloads->end(); i++)  {
			if (i->GetState() == FILEDL_INITIALIZING || i->GetState() == FILEDL_RECEIVING)  {
				i->ProcessDownload();
				downloading = true;
			}
		}

		owner->Unlock();

		// Nothing to download, just sleep
		if (!downloading)
			SDL_Delay(50);
	}

	return 0;
}

//
// Single file download
//

// HINT: Lock() must be called before accessing any class members to ensure thread safety!

//////////////////
// Start the transfer
void CFileDownload::Start(const std::string& filename, const std::string& dest_dir)
{
	// Checks
	if (filename.size() == 0)  {
		SetDlError(FILEDL_ERROR_NO_FILE);
		return;
	}

	if (dest_dir.size() == 0)  {
		SetDlError(FILEDL_ERROR_NO_DEST);
		return;
	}
		
	if (tDownloadServers == NULL)  {
		SetDlError(FILEDL_ERROR_NO_SERVER);
		return;
	}

	// Stop & clear any previous download
	Stop();

	Lock();

	// Fill in the info
	sFileName = filename;
	if (*dest_dir.rbegin() != '/' && *dest_dir.rbegin() != '\\')
		sDestPath = dest_dir + "/" + filename;
	else
		sDestPath = dest_dir + filename;

	// Create the file
	tFile = OpenGameFile(sDestPath, "wb");
	if (tFile == NULL)  {
		SetDlError(FILEDL_ERROR_SAVING);
		Unlock();
		return;
	}

	// Try to download from first server in the list
	iCurrentServer = 0;
	tHttp.RequestData((*tDownloadServers)[iCurrentServer] + filename);

	// Set the state to initializing
	iState = FILEDL_INITIALIZING;

	Unlock();
}

/////////////////
// Stop the transfer and clear everything
void CFileDownload::Stop()
{
	Lock();

	// Cancel the transfer
	if (tHttp.RequestedData())
		tHttp.CancelProcessing();

	// Clear
	sFileName = "";
	sDestPath = "";
	if (tFile)  {
		fclose(tFile);
		remove(GetFullFileName(sDestPath).c_str());
	}
	tFile = NULL;
	iCurrentServer = 0;
	iState = FILEDL_NO_FILE;

	Unlock();

	SetDlError(FILEDL_ERROR_NO_ERROR);
}

/////////////////
// Process the downloading
void CFileDownload::ProcessDownload()
{
	Lock();

	// Check that there has been no error yet
	if (tError.iError != FILEDL_ERROR_NO_ERROR)  {
		Unlock();
		return;
	}

	// Process the HTTP
	int res = tHttp.ProcessRequest();

	Unlock();

	switch (res)  {
	// Still processing
	case HTTP_PROC_PROCESSING:
		Lock();
		iState = FILEDL_RECEIVING;
		Unlock();
		break;

	// Error
	case HTTP_PROC_ERROR:
		// If the file could not be found, try another server
		Lock();
		if (tHttp.GetError().iError == HTTP_FILE_NOT_FOUND)  {
			iCurrentServer++;
			if ((size_t)iCurrentServer < tDownloadServers->size())  {
				tHttp.RequestData((*tDownloadServers)[iCurrentServer] + sFileName);  // Request the file
				iState = FILEDL_INITIALIZING;
				Unlock();
				break;  // Don't set the error
			}
		}
		Unlock();

		SetHttpError(tHttp.GetError());
		Lock();
		iState = FILEDL_ERROR;
		Unlock();
		break;

	// Finished
	case HTTP_PROC_FINISHED:
		Lock();
		iState = FILEDL_FINISHED;

		// Save the data in the file
		if (tFile)  {
			if (fwrite(tHttp.GetData().data(), tHttp.GetData().size(), 1, tFile) != tHttp.GetData().size())
				SetDlError(FILEDL_ERROR_SAVING);
			fclose(tFile);
			tFile = NULL;
		} else {
			SetDlError(FILEDL_ERROR_SAVING);
		}

		Unlock();
		break;
	}

}

/////////////////
// Set downloading error
void CFileDownload::SetDlError(int id)
{
	Lock();
	tError.iError = id;
	tError.sErrorMsg = sDownloadErrors[id];
	tError.tHttpError = tHttp.GetError();
	Unlock();
}

/////////////////
// Set downloading error (HTTP problem)
void CFileDownload::SetHttpError(HttpError err)
{
	Lock();
	tError.iError = FILEDL_ERROR_HTTP;
	tError.sErrorMsg = "HTTP Error: " + err.sErrorMsg;
	tError.tHttpError = err;
	Unlock();
}

/////////////////
// Get the filename (thread safe)
std::string CFileDownload::GetFileName()
{
	Lock();
	std::string res = sFileName;
	Unlock();

	return res;
}

/////////////////
// Get the state (thread safe)
int CFileDownload::GetState()
{
	Lock();
	int res = iState;
	Unlock();

	return res;
}

////////////////
// Get the file downloading progress (thread safe)
byte CFileDownload::GetProgress()
{
	Lock();
	byte res = 0;
	if (tHttp.GetDataLength() != 0)
		res = (byte)MIN(100, tHttp.GetReceivedDataLen() * 100 / tHttp.GetDataLength());
	Unlock();

	return res;
}

////////////////
// Get the download error (thread safe)
DownloadError CFileDownload::GetError()
{
	Lock();
	DownloadError res = tError;
	Unlock();

	return res;
}

//
// File downloader
//

/////////////////
// Constructor
CFileDownloader::CFileDownloader()
{
	// Load the list of available servers
	FILE *fp = OpenGameFile("cfg/downloadservers.txt", "r");
	if (fp)  {
		while (!feof(fp))  {
			std::string server = ReadUntil(fp, '\n');

			// Check (server with less than 3 characters cannot be valid)
			if (server.size() < 3)
				continue;

			// Delete CR character if present
			if (*server.rbegin() == '\r')
				server.erase(server.size() - 1);

			// Trim & add the server
			TrimSpaces(server);
			tDownloadServers.push_back(server);

			// Skip the LF character
			fseek(fp, ftell(fp)+1, SEEK_SET);
		}
	}

	// Initialize the values
	tThread = NULL;
	bBreakThread = false;

	// Create the thread
	tMutex = SDL_CreateMutex();
	tThread = SDL_CreateThread(DownloadThreadMain, this);
}

///////////////
// Destructor
CFileDownloader::~CFileDownloader()
{
	Lock();
	bBreakThread = true;
	Unlock();

	SDL_WaitThread(tThread, NULL);
	SDL_DestroyMutex(tMutex);
}

//////////////
// Add and start a download
void CFileDownloader::StartFileDownload(const std::string& filename, const std::string& dest_dir)
{
	Lock();

	// Add the download
	CFileDownload new_dl = CFileDownload(&tDownloadServers, tDownloads.size());

	tDownloads.push_back(new_dl);

	// Find the newly added download and start it
	for (std::list<CFileDownload>::iterator i = tDownloads.begin(); i != tDownloads.end(); i++)  {
		if (i->GetID() == tDownloads.size()-1)  {
			i->Start(filename, dest_dir);
			break;
		}
	}

	Unlock();

}

/////////////
// Cancel a download
void CFileDownloader::CancelFileDownload(const std::string& filename)
{
	Lock();

	// Find the download and remove it, the destructor will stop the download automatically
	for (std::list<CFileDownload>::iterator i = tDownloads.begin(); i != tDownloads.end(); i++)
		if (i->GetFileName() == filename)  {
			tDownloads.erase(i);
			break;
		}

	Unlock();
}

//////////////
// Returns true if the file has been successfully downloaded
bool CFileDownloader::IsFileDownloaded(const std::string& filename)
{
	Lock();

	for (std::list<CFileDownload>::iterator i = tDownloads.begin(); i != tDownloads.end(); i++)
		if (i->GetFileName() == filename)  {
			Unlock();
			return i->GetState() == FILEDL_FINISHED;
		}

	Unlock();

	return false;
}

//////////////
// Returns the download error for the specified file
DownloadError CFileDownloader::FileDownloadError(const std::string& filename)
{
	Lock();

	for (std::list<CFileDownload>::iterator i = tDownloads.begin(); i != tDownloads.end(); i++)
		if (i->GetFileName() == filename)  {
			Unlock();
			return i->GetError();
		}

	Unlock();

	// Not found, create a default "no error"
	DownloadError err;
	err.iError = FILEDL_ERROR_NO_ERROR;
	err.sErrorMsg = sDownloadErrors[FILEDL_ERROR_NO_ERROR];
	err.tHttpError.iError = HTTP_NO_ERROR;
	err.tHttpError.sErrorMsg = "No error";
	return err;
}

///////////////
// Get the file download progress in percents
byte CFileDownloader::GetFileProgress(const std::string& filename)
{
	Lock();

	for (std::list<CFileDownload>::iterator i = tDownloads.begin(); i != tDownloads.end(); i++)
		if (i->GetFileName() == filename)  {
			Unlock();
			return i->GetProgress();
		}

	Unlock();

	return 0;
}

////////////////
// Returns break thread signal
bool CFileDownloader::ShouldBreakThread()
{
	Lock();
	bool res = bBreakThread;
	Unlock();
	return res;
}



void CFileDownloaderInGame::setFileToSend( const std::string & name, const std::string & data )
{
	tState = S_SEND;
	iPos = 0;
	sFilename = name;
	Compress( sFilename + '\0' + data, &sData );
	printf("CFileDownloaderInGame::setFileToSend() filename %s data.size() %i compressed %i\n", sFilename.c_str(), data.size(), sData.size() );
};

void CFileDownloaderInGame::setFileToSend( const std::string & path )
{
	tState = S_SEND;
	iPos = 0;
	sFilename = path;

	FILE * ff = OpenGameFile( sFilename , "r" );
	if( ff == NULL )
	{
		tState = S_ERROR;
		return;
	};
	char buf[16384];
	std::string data = "";
	
	while( ! feof( ff ) )
	{
		int readed = fread( buf, 1, sizeof(buf), ff );
		data.append( buf, readed );
	};
	fclose( ff );

	Compress( sFilename + '\0' + data, &sData );
	printf("CFileDownloaderInGame::setFileToSend() filename %s data.size() %i compressed %i\n", sFilename.c_str(), data.size(), sData.size() );
};

enum { MAX_DATA_CHUNK = 254 };	// UCHAR_MAX - 1, client and server should have this equal
bool CFileDownloaderInGame::receive( CBytestream * bs )
{
	uint chunkSize = bs->readByte();
	if( chunkSize == 0 )	// Ping packet with zero data - do not change downloader state
		return false;
	if( tState == S_FINISHED )
	{
		tState = S_RECEIVE;
		iPos = 0;
		sFilename = "";
		sData = "";
	};
	if( tState != S_RECEIVE )
	{
		tState = S_ERROR;
		return true;	// Receive finished (due to error)
	};
	//printf("CFileDownloaderInGame::receive() chunk %i\n", chunkSize);
	bool Finished = false;
	if( chunkSize != MAX_DATA_CHUNK )
	{
		Finished = true;
		if( chunkSize > MAX_DATA_CHUNK )
			chunkSize = MAX_DATA_CHUNK;
	}
	sData.append( bs->readData(chunkSize) );
	if( Finished )
	{
		iPos = 0;
		tState = S_ERROR;
		if( Decompress( sData, &sFilename ) )
		{
			tState = S_FINISHED;
			std::string::size_type f = sFilename.find('\0');
			if( f == std::string::npos )
				tState = S_ERROR;
			else
			{
				sData.assign( sFilename, f+1, sFilename.size() - (f+1) );
				sFilename.resize( f );
				printf("CFileDownloaderInGame::receive() filename %s sData.size() %i\n", sFilename.c_str(), sData.size());
			};
		};
		processFileRequests();
		return true;	// Receive finished
	};
	return false;
};

bool CFileDownloaderInGame::send( CBytestream * bs )
{
	if( tState != S_SEND )
	{
		tState = S_ERROR;
		return true;	// Send finished (due to error)
	};
	uint chunkSize = MIN( sData.size() - iPos, MAX_DATA_CHUNK );
	if( sData.size() - iPos == MAX_DATA_CHUNK )
		chunkSize++;
	bs->writeByte( chunkSize );
	bs->writeData( sData.substr( iPos, MIN( chunkSize, MAX_DATA_CHUNK ) ) );
	iPos += chunkSize;
	//printf("CFileDownloaderInGame::send() %i/%i\n", iPos, sData.size() );
	if( chunkSize != MAX_DATA_CHUNK )
	{
		tState = S_FINISHED;
		iPos = 0;
		return true;	// Send finished
	};
	return false;
};

void CFileDownloaderInGame::sendPing( CBytestream * bs ) const
{
	bs->writeByte( 0 );
};

void CFileDownloaderInGame::allowFileRequest( bool allow ) 
{ 
	bAllowFileRequest = allow;
	if( ! bAllowFileRequest )
		reset();	// Stop uploading any files
};

void CFileDownloaderInGame::requestFile( const std::string & path, bool retryIfFail )
{
	setFileToSend( "GET:", path );
	if( retryIfFail )
		tRequestedFiles.insert( path );
};

bool CFileDownloaderInGame::requestFilesPending()
{
	if( tRequestedFiles.empty() )
		return false;
	if( getState() == S_FINISHED || getState() ==  S_ERROR )
		reset();
	else
		return true;	// Receiving or sending in progress
		
	requestFile( * tRequestedFiles.begin() );
	return true;
};

void CFileDownloaderInGame::requestFileInfo( const std::string & path )
{
	setFileToSend( "STAT:", path );
};

void CFileDownloaderInGame::processFileRequests()
{
	// Process received files
	if( tRequestedFiles.find( getFilename() ) != tRequestedFiles.end() )
		tRequestedFiles.erase( getFilename() );
	
	// Process file sending requests
	if( ! bAllowFileRequest )
		return;
	if( getFilename() == "GET:" )
	{
		if( ! isPathValid( getData() ) )
		{
			printf( "CFileDownloaderInGame::processFileRequests(): invalid filename \"%s\"\n", getData().c_str() );
			return;
		};
		std::string fname;
		GetExactFileName( getData(), fname );
		struct stat st;
		if( stat( fname.c_str(), &st ) != 0 )
		{
			printf( "CFileDownloaderInGame::processFileRequests(): cannot stat file \"%s\"\n", getData().c_str() );
			return;
		};
		if( S_ISREG( st.st_mode ) )
		{
			setFileToSend( getData() );
		};
		if( S_ISDIR( st.st_mode ) )
		{
			printf( "CFileDownloaderInGame::processFileRequests(): cannot send dir \"%s\" - not implemented\n", getData().c_str() );
			// TODO: Not implemented
			return;
		};
	};
	if( getFilename() == "STAT:" )
	{
		if( ! isPathValid( getData() ) )
		{
			printf( "CFileDownloaderInGame::processFileRequests(): invalid filename \"%s\"\n", getData().c_str() );
			return;
		};
		printf( "CFileDownloaderInGame::processFileRequests(): cannot send file statistics \"%s\" - not implemented\n", getData().c_str() );
		// TODO: Get file info - size and CRC32 or MD5 - not implemented yet
		return;
	};
};

// Valid filename symbols
#define S_LETTER_UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define S_LETTER_LOWER "abcdefghijklmnopqrstuvwxyz"
#define S_LETTER S_LETTER_UPPER S_LETTER_LOWER
#define S_NUMBER "0123456789"
#define S_SYMBOL "/. -_&+"	// No "\\" symbol
const char * invalid_file_names [] = { "CON", "PRN", "AUX", "NUL", 
	"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", 
	"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };

bool CFileDownloaderInGame::isPathValid( const std::string & path )
{
	if( path == "" )
		return false;
	if( path.find_first_not_of( S_LETTER S_NUMBER S_SYMBOL ) !=	std::string::npos )
		return false;
	if( path[0] == '/' || path[0] == ' ' )
		return false;
	if( path[path.size()-1] == ' ' )
		return false;
	if( path.find( ".." ) != std::string::npos ||
		path.find( "//" ) != std::string::npos ||
		path.find( "./" ) != std::string::npos ||
		path.find( "/." ) != std::string::npos )	// Okay, "~/.OpenLieroX/" is valid path, fail it anyway
		return false;
	if( stringcasefind( path, "cfg/" ) == 0 )	// Config dir is forbidden - some passwords may be stored here
		return false;
	for( uint f=0; f < path.size(); )
	{
		uint f1 = path.find_first_of(S_SYMBOL, f);
		if( f1 == std::string::npos )
			f1 = path.size();
		std::string word = path.substr( f, f1-f );
		for( uint f2=0; f2<sizeof(invalid_file_names)/sizeof(invalid_file_names[0]); f2++ )
		{
			if( stringcasecmp( word, invalid_file_names[f2] ) == 0 )
				return false;
		};
		if( f1 == path.size() )
			break;
		f = path.find_first_not_of(S_SYMBOL, f1);
		if( f == std::string::npos )	// Filename cannot end with a dot
			return false;
	};
	return true;
};

