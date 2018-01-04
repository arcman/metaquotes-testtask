#include "CLogReader.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

CLogReader::CLogReader()
{
	m_filePtr = NULL;
	m_filterStr = NULL;
	m_bufferPtr = NULL;
	m_bufferSize = 0;
	m_lineFeedPtr = NULL;
	m_blockPtr = NULL;
	m_linePtr = NULL;
	m_bytesReaded = 0;
}

CLogReader::~CLogReader()
{
	free(m_filterStr);
	free(m_bufferPtr);
	Close();
}

// Opens a file on the specified path
// Returns false on error
bool CLogReader::Open(const char *path)
{
	if (path)
	{
		m_filePtr = fopen(path, "rb");
	}

	return m_filePtr;
}

// Closes a file
void CLogReader::Close(void)
{
	if (m_filePtr)
	{
		fclose(m_filePtr);
		m_filePtr = NULL;
	}
}

// Initialize filter (pattern) string
// Returns false on error
bool CLogReader::SetFilter(const char *filter)
{
	if (filter)
	{
		// Allocate internal buffer and copy filter string to them

		size_t len = strnlen(filter, m_maxFilterSize);

		if (len == m_maxFilterSize)
		{
			return false;
		}

		char *tmpPtr = static_cast<char *>(realloc(m_filterStr, len + 1));
		
		if (tmpPtr == NULL)
		{
			return false;
		}

		m_filterStr = tmpPtr;

		memcpy(m_filterStr, filter, len + 1);

		if (m_bufferPtr == NULL)
		{
			// Prepare an internal buffer in which the data from the file will be read
			tmpPtr = static_cast<char *>(realloc(m_bufferPtr, m_bufferSize + m_blockSize));
			
			if (tmpPtr == NULL)
			{
				return false;
			}

			// Initialize class members to work with the new buffer
			m_bufferPtr = tmpPtr;
			m_bufferSize += m_blockSize;
			m_blockPtr = m_bufferPtr;
			m_linePtr = m_bufferPtr;
			m_lineFeedPtr = m_bufferPtr;
		}

		return true;
	}

	return false;
}

// Reads file to the internal buffer and returns pointer to the next line of text
//   using linePtr. Copies length of the line to lineLength.
// Returns false as a return value if end of file reached or error
bool CLogReader::GetNextLineRaw(char **linePtr, size_t *lineLength)
{
	if (linePtr == NULL || lineLength == NULL || m_bufferPtr == NULL)
	{
		return false;
	}

	do
	{
		if (m_lineFeedPtr != m_bufferPtr)
		{
			// Setup m_linePtr to the beginning of new text line right after '\n'
			//   if we have at least one extra byte in the buffer
			if (m_lineFeedPtr - m_blockPtr + 1 < m_bytesReaded)
			{
				m_linePtr = m_lineFeedPtr + 1;
			}
			else
			{
				// No space left in the buffer, we need to read new block of data
				m_linePtr = m_blockPtr;
				m_lineFeedPtr = m_blockPtr;
			}
		}
		
		// Fills the buffer with a data from the file when required
		if (m_lineFeedPtr == m_linePtr)
		{
			m_bytesReaded = fread(m_blockPtr, 1, m_blockSize, m_filePtr);
		}

		// Searches for the next line feed symbol ('\n')
		if (m_lineFeedPtr = static_cast<char *>(memchr(m_linePtr, '\n', m_bytesReaded - (m_linePtr - m_blockPtr))))
		{
			// Prepare pointer to the new text line found and calculate line lenght

			*m_lineFeedPtr = '\0';

			*lineLength = m_lineFeedPtr - m_linePtr;

			// Handling CRLF ("\r\n") case
			if (m_lineFeedPtr != m_bufferPtr && *(m_lineFeedPtr - 1) == '\r')
			{
				*(m_lineFeedPtr - 1) = '\0';
				(*lineLength)--;
			}

			*linePtr = m_linePtr;
			return true;
		}
		else if (m_bytesReaded == m_blockSize)
		{
			// No line feed symbol found, we need to read next block of data from file
			// Here is two cases possible:
			if (m_linePtr == m_bufferPtr)
			{
				// Line too long to fit the buffer, so truncate them
				m_blockPtr[m_blockSize - 1] = '\0';
				*linePtr = m_linePtr;
				*lineLength = m_blockSize - 1;
				m_linePtr = m_blockPtr;
				m_lineFeedPtr = m_blockPtr;
				return true;
			}
			else
			{
				// End of block reached - rewind to beginning of the current line
				// Will read them again with new block
				fseek(m_filePtr, -1L * (m_bytesReaded - (m_linePtr - m_blockPtr)), SEEK_CUR);
				m_linePtr = m_blockPtr;
				m_lineFeedPtr = m_blockPtr;
			}
		}
		else
		{
			if (m_bytesReaded > 0)
			{
				// End of file reached, but no line feed ('\n') symbol at the end of file
				m_blockPtr[m_bytesReaded] = '\0';
				*linePtr = m_linePtr;
				*lineLength = m_bytesReaded - (m_linePtr - m_blockPtr);				
				return true;
			}
		}
	} while (m_bytesReaded > 0);

	return false;
}

// Compare strings to each other. Second string might contains wildcards '*' and '?'
// Returns true if the first string matched to the second
// Modified solution from http://yucoding.blogspot.com/2013/02/leetcode-question-123-wildcard-matching.html
bool CLogReader::MatchString(const char *str, const char *filter)
{
	const char *asterisk = NULL;
	const char *ss = str;
	
	while (*str) 
	{
		// Advancing both pointers when both characters match or '?' found in pattern
		// Note that *filter will not advance beyond it's length
		if (*filter == '?' || *filter == *str)
		{ 
			str++;
			filter++;
			continue;
		}
		
		//	* found in pattern, track index of *, only advancing pattern pointer
		if (*filter == '*')
		{
			asterisk = filter++;
			ss = str;
			continue;
		}

		// Current characters didn't match, last pattern pointer was *, current pattern pointer is not *
		// So only advancing pattern pointer
		if (asterisk)
		{
			filter = asterisk + 1;
			str = ++ss;
			continue;
		}

		// Current pattern pointer is not asterisk, last patter pointer was not *
		// Strings do not match
		return false;
	}

	// Check for remaining characters in pattern
	while (*filter == '*') 
	{ 
		filter++;
	}

	// Return true if the end of the pattern's string was reached
	return !*filter;
}

// Copies to the buffer next text line matched to the pattern (filter) string
// Returns false on error or if end of the file was reached
bool CLogReader::GetNextLine(char *buf, const int bufsize) 
{
	if (buf == NULL || bufsize <= 1 || m_filterStr == NULL)
	{
		return false;
	}

	char *line = NULL;
	size_t len = 0;
	
	// Read file line by line
	while (GetNextLineRaw(&line, &len))
	{
		if (MatchString(line, m_filterStr))
		{
			// Copy current text line to the outgoing buffer if it's matched with pattern
			//   and buffer has enought space to store whole line
			if (bufsize > len)
			{
				memcpy(buf, line, len + 1);
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}
