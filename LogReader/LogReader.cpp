// LogReader.cpp : Defines the entry point for the console application.
//

#include "CLogReader.h"

uint64_t GetMcsCnt(void)
{
	LARGE_INTEGER fq, t;
	QueryPerformanceFrequency(&fq);
	QueryPerformanceCounter(&t);
	return (1000000 * t.QuadPart) / fq.QuadPart;
}

void ShowUsage(void)
{
	fprintf(stderr, "Usage:\nlogreader <log_file> <filter>\n");
}

int main(int argc, char *argv[])
{
	char		buf[4096] = { 0 };
	uint64_t	ts;
	int			i = 0;

	if (argc < 3)
	{
		ShowUsage();
		return 1;
	}

	ts = GetMcsCnt();

	CLogReader	logreader;
	
	if (!logreader.Open(argv[1]))
	{	
		fprintf(stderr, "Failed to open file '%s'\n", argv[1]);
		return 1;
	}

	if (!logreader.SetFilter(argv[2]))
	{
		fprintf(stderr, "Failed to set filter '%s'\n", argv[2]);
		return 1;
	}

	while (logreader.GetNextLine(buf, sizeof(buf)))
	{
		fprintf(stdout, "%d: '%s'\n", i++, buf);
	}

	logreader.Close();

	ts = GetMcsCnt() - ts;

	fprintf(stdout, "ts: %llu ms\n", ts/1000);

    return 0;
}
