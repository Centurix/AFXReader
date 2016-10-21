// AFXReader.cpp 
// Author: Chris Read
// Date: 7/8/2001
// Description:
// Reads a windows WAVE file and converts track 3 from 2 remixes by AFX from
// linear frequency modulated data into 8-bit binary data.

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define BIT_LOWER  1500
#define BIT_HIGHER 2000

int main(int argc, char* argv[])
{
	HMMIO		hInputFile;
	FILE*		lpfOutputFile;
	MMCKINFO	mmckinfoParent;
	MMCKINFO	mmckinfoSubchunk;
	DWORD		dwFmtSize;
	DWORD		dwDataSize;
	WAVEFORMAT*	pFormat;
	int			iBitWidth;
	char		buffer[400];
	int			iTemp;
	int			iRead;
	short		wSample=0; // Signed word
	int			iZeroCrossCount;
	short		wLastSample;
	int			iSampleCount;
	double		dFrequency;
	double		dAverage;
	int			iAverageCounter;
	int			iBitCounter;

	if(argc < 2)
	{
		printf("Usage AFXReader.exe [WAVfile.wav]\n");
		exit(0);
	}
	
	if((hInputFile = mmioOpen(argv[1],NULL,MMIO_READ)) == NULL)
	{
		printf("Cannot open file\n");
		exit(1);
	}
	else
	{
		// Check to see if this is a WAVE file
		mmckinfoParent.fccType = mmioFOURCC('W','A','V','E');
		if(mmioDescend(hInputFile,(LPMMCKINFO)&mmckinfoParent,NULL,MMIO_FINDRIFF))
		{
			printf("File is not a WAVE file\n");
			mmioClose(hInputFile,NULL);
			exit(1);
		}

		// It's a WAVE file. Lets find out some info about it.
		// Descend into the RIFF subchunk
		mmckinfoSubchunk.ckid = mmioFOURCC('f','m','t',' ');
		if(mmioDescend(hInputFile,&mmckinfoSubchunk,&mmckinfoParent,MMIO_FINDCHUNK))
		{
			printf("File does not have a fmt chunk\n");
			mmioClose(hInputFile,NULL);
			exit(1);
		}

		dwFmtSize = mmckinfoSubchunk.cksize;

		pFormat = (WAVEFORMAT*)malloc(dwFmtSize);

		if(mmioRead(hInputFile,(HPSTR)pFormat,dwFmtSize) != (long)dwFmtSize)
		{
			printf("Failed to read fmt chunk\n");
			free(pFormat);
			mmioClose(hInputFile,NULL);
			exit(1);
		}

		// Display the WAVE file information
		printf("WAVE Format: %ldHz, ",pFormat->nSamplesPerSec);
		if(pFormat->nChannels==1)
			printf("Mono, ");
		else
			printf("Stereo, ");
		if(pFormat->wFormatTag==WAVE_FORMAT_PCM)
			printf("PCM, ");
		else
			printf("Unknown, ");

		iBitWidth = (pFormat->nBlockAlign / pFormat->nChannels) * 8;

		printf("%d Bits\n",iBitWidth);

		// Ascend
		mmioAscend(hInputFile,&mmckinfoSubchunk,0);

		mmckinfoSubchunk.ckid = mmioFOURCC('d','a','t','a');
		if(mmioDescend(hInputFile,&mmckinfoSubchunk,&mmckinfoParent,MMIO_FINDCHUNK))
		{
			printf("WAVE has no data subchunk\n");
			free(pFormat);
			mmioClose(hInputFile,NULL);
			exit(1);
		}

		dwDataSize = mmckinfoSubchunk.cksize;

		if(dwDataSize==0L)
		{
			printf("DATA chunk contains no data\n");
			free(pFormat);
			exit(1);
		}


		// Do the work!
		printf("Processing, please wait...\n");

		lpfOutputFile = fopen("frequencies.txt","w");

		iRead = mmioRead(hInputFile,buffer,pFormat->nBlockAlign * 50);

		iRead = mmioRead(hInputFile,buffer,400);

		iBitCounter = 0;

		// Allocate a buffer and read the samples in
		iZeroCrossCount = 0;
		iSampleCount = 0;
		iAverageCounter = 0;
		dAverage = 0;

		while(iRead)
		{
			// Calculate the frequencies in this 100 bytes
			for(iTemp=0;iTemp<iRead;iTemp+=pFormat->nBlockAlign)
			{
				wLastSample = wSample;
				wSample = MAKEWORD(buffer[iTemp],buffer[iTemp+1]);
				if((wLastSample > 0 && wSample < 0) || (wLastSample < 0 && wSample > 0))
				{
					// Crossover found!
					iZeroCrossCount++;
					if(iZeroCrossCount==2)
					{
						// Two crossovers indicates a single cycle
						// Work out the frequency
						// Samples per second / iSample count (roughly)
						dFrequency = pFormat->nSamplesPerSec / iSampleCount;
						
						// Write out the number of samples in this cycle
						fprintf(lpfOutputFile,"%d\n",iSampleCount);

						iZeroCrossCount = 0; // Because we've already found the crossover for the next sample
						iSampleCount=1; // And we're already one sample in

						// Work out the average
//						iAverageCounter++;
//						if(iAverageCounter==4) // 6 cycles per bit
//						{
//							if(dFrequency > BIT_HIGHER)
//								fprintf(lpfOutputFile,"1");
//							else if(dFrequency < BIT_HIGHER && dFrequency > BIT_LOWER)
//								fprintf(lpfOutputFile,"0");
//							else if(dFrequency < BIT_LOWER)
//								fprintf(lpfOutputFile,"\n");
//							iAverageCounter=0;
//						}
					}
				}
				iSampleCount++;
			}
			iRead = mmioRead(hInputFile,buffer,400);
		}

		fclose(lpfOutputFile);

		printf("Complete!\n");

		free(pFormat);

		mmioClose(hInputFile,NULL);
	}
	return 0;
}
