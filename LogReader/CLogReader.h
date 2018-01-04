#pragma once

#include <cstdio>

// Reads text log file line by line and matches each line to pattern string
// Limitations:
// 1. Log file should't grow at the same time
// 2. Max lenght of a line (log record) restricted to 64k
// 3. File should be a text log with '\n' or "\r\n" as line delimiters
//
class CLogReader
{
public:
	CLogReader();
	~CLogReader();

	// Opens a file on the specified path
	// Returns false on error
	bool Open(const char *path);

	// Closes a file
	void Close(void);

	// Initialize filter (pattern) string
	// Returns false on error
	bool SetFilter(const char *filter);

	// Copies to the buffer next text line matched to the pattern (filter) string
	// Returns false on error or if end of the file was reached
	bool GetNextLine(char *buf, const int bufsize);
private:
	FILE *m_filePtr;
	char *m_filterStr;
	char *m_bufferPtr;
	size_t m_bufferSize;
	const size_t m_blockSize = 65536;
	const size_t m_maxFilterSize = 1024;

	char *m_lineFeedPtr, *m_blockPtr, *m_linePtr;
	size_t m_bytesReaded;

	// Reads file to the internal buffer and returns pointer to the next line of text
	//   using linePtr. Copies length of the line to lineLength.
	// Returns false as a return value if end of file reached or error
	bool CLogReader::GetNextLineRaw(char **linePtr, size_t *lineLength);

	// Compare strings to each other. Second string might contains wildcards '*' and '?'
	// Returns true if the first string matched to the second
	bool CLogReader::MatchString(const char *str, const char *filter);
};
