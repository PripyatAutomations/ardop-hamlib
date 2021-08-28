// ARDOP TNC ARQ Code
//

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#include "ARDOPC.h"

#ifdef TEENSY
#define PKTLED LED3		// flash when packet received

extern unsigned int PKTLEDTimer;
#endif


extern UCHAR bytData[];
extern int intLastRcvdFrameQuality;
extern int intRmtLeaderMeasure;
extern BOOL blnAbort;
extern int intRepeatCount;
extern unsigned int dttLastFECIDSent;
extern unsigned int tmrSendTimeout;
extern BOOL blnFramePending;
extern int dttLastBusyTrip;
extern int dttPriorLastBusyTrip;
extern int dttLastBusyClear;

extern int unackedByteCount;


int intLastFrameIDToHost = 0;
int	intLastFailedFrameID = 0;
int	intLastARQDataFrameToHost = -1;
int	intShiftUpDn = 0;
int intFrameTypePtr = 0;	 // Pointer to the current data mode in bytFrameTypesForBW() 
int	intRmtLeaderMeas = 0;
int intTrackingQuality = -1;
UCHAR bytLastARQDataFrameSent = 0;  // initialize to an improper data frame
UCHAR bytLastARQDataFrameAcked = 0;  // initialize to an improper data frame
void ClearTuningStats();


void ClearQualityStats();
void updateDisplay();
void DrawTXMode(const char * TXMode);
UCHAR GenCRC8(UCHAR * Data, int Len);
VOID RequeueData();
VOID ResetTXState();
VOID ResetPSNState();
VOID EncodeAndSendMulticarrierACK(UCHAR bytSessionID, int LeaderLength);


int bytQDataInProcessLen = 0;		// Lenght of frame to send/last sent

BOOL blnLastFrameSentData = FALSE;

extern char CarrierOk[10];
extern int LastDataFrameType;	
extern BOOL blnARQDisconnect;
extern const short FrameSize[64];

int intNAKLoQThresholds[6]; // the following two integer arrays hold the quality thresholds for making faster mode shifts, one value for each data type. 
int intACKHiQThresholds[6];

// ARQ State Variables

char AuxCalls[10][10] = {0};
int AuxCallsLength = 0;

int intBW;			// Requested connect speed
int intSessionBW;	// Negotiated speed

const char ARQBandwidths[9][12] = {"200", "500", "2500", "UNDEFINED"};
enum _ARQSubStates ARQState;

const char ARQSubStates[10][11] = {"None", "ISSConReq", "ISSConAck", "ISSData", "ISSId", "IRSConAck", "IRSData", "IRSBreak", "IRSfromISS", "DISCArqEnd"};

char strRemoteCallsign[10];
char strLocalCallsign[10];
char strFinalIDCallsign[10];
char strGridSquare[10];

UCHAR bytLastARQSessionID;
BOOL blnEnbARQRpt;
BOOL blnListen = TRUE;
BOOL Monitor = TRUE;
BOOL AutoBreak = TRUE;
BOOL blnBREAKCmd = FALSE;
BOOL BusyBlock = FALSE;

UCHAR bytPendingSessionID;
UCHAR bytSessionID = 0x3f;
BOOL blnARQConnected;

UCHAR bytCurrentFrameType = 0;	// The current frame type used for sending
UCHAR * bytFrameTypesForBW;		// Holds the byte array for Data modes for a session bandwidth.  First is most robust, last is fastest
int * bytFrameLengthsForBW;	// Holds the byte count for Data modes for a session bandwidth.
int * upThresholds;			// Gear shift up %
int * downThresholds;		// Gear shift down %

int bytFrameTypesForBWLength = 0;

int TempMode = -1;			// Used to shift to more robust mode for short frames
int SavedMode = -1;

UCHAR * bytShiftUpThresholds;
int bytShiftUpThresholdsLength;

BOOL blnPending;
int dttTimeoutTrip;
int intLastARQDataFrameToHost;
int intReceivedLeaderLen;
unsigned int tmrFinalID = 0;
unsigned int tmrIRSPendingTimeout = 0;
unsigned int tmrPollOBQueue;
UCHAR bytLastReceivedDataFrameType;
BOOL blnDISCRepeating;

int intRmtLeaderMeas;

int	intOBBytesToConfirm = 0;	// remaining bytes to confirm  
int	intBytesConfirmed = 0;		// Outbound bytes confirmed by ACK and squenced
int	intReportedLeaderLen = 0;	// Zero out the Reported leader length the length reported to the remote station 
BOOL blnLastPSNPassed = FALSE;	// the last PSN passed True for Odd, FALSE for even. 
BOOL blnInitiatedConnection = FALSE; // flag to indicate if this station initiated the connection
short dblAvgPECreepPerCarrier = 0; // computed phase error creep
int dttLastIDSent;				// date/time of last ID
int	intTotalSymbols = 0;		// To compute the sample rate error

extern int bytDataToSendLength;
int intFrameRepeatInterval;

extern int intLeaderRcvdMs;	

int intTrackingQuality;
int intNAKctr = 0;
int intACKctr = 0;
UCHAR bytLastACKedDataFrameType;

int EncodeConACKwTiming(UCHAR bytFrameType, int intRcvdLeaderLenMs, UCHAR bytSessionID, UCHAR * bytreturn);
int IRSNegotiateBW(int intConReqFrameType);
int GetNextFrameData(int * intUpDn, UCHAR * bytFrameTypeToSend, UCHAR * strMod, BOOL blnInitialize);
BOOL CheckForDisconnect();
BOOL Send10MinID();
void ProcessPingFrame(char * bytData);
void ProcessCQFrame(char * bytData);

void LogStats();
int ComputeInterFrameInterval(int intRequestedIntervalMS);
BOOL CheckForDisconnect();

// Tuning Stats

int intLeaderDetects;
int intLeaderSyncs;
int intAccumLeaderTracking;
float dblFSKTuningSNAvg;
int intGoodFSKFrameTypes;
int intFailedFSKFrameTypes;
int intAccumFSKTracking;
int intFSKSymbolCnt;
int intGoodFSKFrameDataDecodes;
int intFailedFSKFrameDataDecodes;
int intAvgFSKQuality;
int intFrameSyncs;
int intGoodPSKSummationDecodes;
int intGoodFSKSummationDecodes;
int intGoodAPSKSummationDecodes;
float dblLeaderSNAvg;
int intAccumPSKLeaderTracking;
float dblAvgPSKRefErr;
int intPSKTrackAttempts;
int intAccumPSKTracking;
int intQAMTrackAttempts;
int intAccumQAMTracking;
int intPSKSymbolCnt;
int intQAMSymbolCnt;
int intGoodPSKFrameDataDecodes;
int intFailedPSKFrameDataDecodes;
int intGoodAPSKFrameDataDecodes;
int intFailedAPSKFrameDataDecodes;
int intAvgPSKQuality;
float dblAvgDecodeDistance;
int intDecodeDistanceCount;
int	intShiftUPs;
int intShiftDNs;
unsigned int dttStartSession;
int intLinkTurnovers;
int intEnvelopeCors;
float dblAvgCorMaxToMaxProduct;
int intConReqSN;
int intConReqQuality;
int intTimeouts;

char strLastStringPassedToHost[80] = "";	// Used to suppress duplicate CON Req reports

VOID GetNAKLoQLevels(int intBW)
{
	switch(intBW)
	{
	// These are the thresholds for each data type/bandwidth that determine when to send a NAK (Qual>NAKLoQLevel) or NAKLoQ (Qual<NAKLowQLevel)
	//Preliminary assignments subject to change  TODO: consider optimising using HF simulator

	case 200:

		intNAKLoQThresholds[0] = 0;
		intNAKLoQThresholds[1] = 60;
		intNAKLoQThresholds[2] = 60;
		break;

	case 500:

		intNAKLoQThresholds[0] = 0;
		intNAKLoQThresholds[1] = 55;
		intNAKLoQThresholds[2] = 55;
		intNAKLoQThresholds[3] = 60;
		break;

	case 2500:

		intNAKLoQThresholds[0] = 0;
		intNAKLoQThresholds[1] = 52;
		intNAKLoQThresholds[2] = 50;
		intNAKLoQThresholds[3] = 55;
		intNAKLoQThresholds[4] = 60;
		intNAKLoQThresholds[5] = 65;
		break;
	}
}


// Function to get ACK HiQ thresholds for the ARQ data modes 
VOID GetACKHiQLevels(int intBW)
{
	switch(intBW)
	{
	// These are the thresholds for each data type/bandwidth that determine when to send a NAK (Qual>NAKLoQLevel) or NAKLoQ (Qual<NAKLowQLevel)
	//Preliminary assignments subject to change  TODO: consider optimising using HF simulator

	case 200:

		intACKHiQThresholds[0] = 75;
		intACKHiQThresholds[1] = 80;
		intACKHiQThresholds[2] = 0;
		break;

	case 500:

		intACKHiQThresholds[0] = 72;
		intACKHiQThresholds[1] = 80;
		intACKHiQThresholds[2] = 70;
		intACKHiQThresholds[3] = 0;
		break;

	case 2500:

		intACKHiQThresholds[0] = 66;
		intACKHiQThresholds[1] = 77;
		intACKHiQThresholds[2] = 75;
		intACKHiQThresholds[3] = 80;
		intACKHiQThresholds[4] = 80;
		intACKHiQThresholds[5] = 0;
		break;
	}
}


// Subroutine to compute a 8 bit CRC value and append it to the Data...
/*
UCHAR GenCRC8(char * Data)
{
	//For  CRC-8-CCITT =    x^8 + x^7 +x^3 + x^2 + 1  intPoly = 1021 Init FFFF

	int intPoly = 0xC6; // This implements the CRC polynomial  x^8 + x^7 +x^3 + x^2 + 1
	int intRegister  = 0xFF;
	int i; 
	unsigned int j;
	BOOL blnBit;

	for (j = 0; j < strlen(Data); j++)
	{
		int Val = Data[j];
		
		for (i = 7; i >= 0; i--) // for each bit processing MS bit first
		{
            blnBit = (Val & 0x80) != 0;
			Val = Val << 1;

			if ((intRegister & 0x80) == 0x80)  // the MSB of the register is set
			{
				// Shift left, place data bit as LSB, then divide
				// Register := shiftRegister left shift 1
				// Register := shiftRegister xor polynomial

				if (blnBit) 
					intRegister = 0xFF & (1 + 2 * intRegister);
				else
					intRegister = 0xFF & (2 * intRegister);
                 
				intRegister = intRegister ^ intPoly;
			}
			else
			{
				// the MSB is not set
				// Register is not divisible by polynomial yet.
				// Just shift left and bring current data bit onto LSB of shiftRegister

				if (blnBit)
					intRegister = 0xFF & (1 + 2 * intRegister);
				else
					intRegister = 0xFF & (2 * intRegister);
			}
		}
	}
	return intRegister & 0xFF; // LS 8 bits of Register 

}

*/
int ComputeInterFrameInterval(int intRequestedIntervalMS)
{
	return max(1000, intRequestedIntervalMS + intRmtLeaderMeas);
}


//  Subroutine to Set the protocol state 

void SetARDOPProtocolState(int value)
{
	char HostCmd[24];

	if (ProtocolState == value)
		return;

	ProtocolState = value;

	displayState(ARDOPStates[ProtocolState]);

	newStatus = TRUE;				// report to PTC

        //Dim stcStatus As Status
        //stcStatus.ControlName = "lblState"
        //stcStatus.Text = ARDOPState.ToString

	switch(ProtocolState)
	{
	case DISC:

		blnARQDisconnect = FALSE; // always clear the ARQ Disconnect Flag from host.
		//stcStatus.BackColor = System.Drawing.Color.White
		blnARQConnected = FALSE;
		blnPending = FALSE;
		ClearDataToSend();
		SetLED(ISSLED, FALSE);
		SetLED(IRSLED, FALSE);
		displayCall(0x20, "");

		break;

	case FECRcv:
		//stcStatus.BackColor = System.Drawing.Color.PowderBlue
		break;
		
	case FECSend:

		InitializeConnection();
		intLastFrameIDToHost = -1;
		intLastFailedFrameID = -1;
		//ReDim bytFailedData(-1)
		//stcStatus.BackColor = System.Drawing.Color.Orange
		break;

        //    Case ProtocolState.IRS
        //        stcStatus.BackColor = System.Drawing.Color.LightGreen

	case ISS:
	case IDLE:

		blnFramePending = FALSE;	//  Added 0.6.4 to ensure any prior repeating frame is cancelled before new data. 
		blnEnbARQRpt = FALSE;
		SetLED(ISSLED, TRUE);
		SetLED(IRSLED, FALSE);
  
        //        stcStatus.BackColor = System.Drawing.Color.LightSalmon

		break;

	case IRS:
	case IRStoISS:

		SetLED(IRSLED, TRUE);
		SetLED(ISSLED, FALSE);
		bytLastACKedDataFrameType = 0;	// Clear on entry to IRS or IRS to ISS states. 3/15/2018

		break;


        //    Case ProtocolState.IDLE
        //        stcStatus.BackColor = System.Drawing.Color.NavajoWhite
        //    Case ProtocolState.OFFLINE
         //       stcStatus.BackColor = System.Drawing.Color.Silver
	}
	//queTNCStatus.Enqueue(stcStatus)

	sprintf(HostCmd, "NEWSTATE %s ", ARDOPStates[ProtocolState]);
	QueueCommandToHost(HostCmd);
}

 

//  Function to Get the next ARQ frame returns TRUE if frame repeating is enable 

BOOL GetNextARQFrame()
{
	//Dim bytToMod(-1) As Byte

	char HostCmd[80];

	if (blnAbort)  // handles ABORT (aka Dirty Disconnect)
	{
		//if (DebugLog) ;(("[ARDOPprotocol.GetNextARQFrame] ABORT...going to ProtocolState DISC, return FALSE")

		ClearDataToSend();
		
		SetARDOPProtocolState(DISC);
		InitializeConnection();
		blnAbort = FALSE;
		blnEnbARQRpt = FALSE;
		blnDISCRepeating = FALSE;
		intRepeatCount = 0;

		return FALSE;
	}

	if (blnDISCRepeating)	// handle the repeating DISC reply 
	{
		intRepeatCount += 1;
		blnEnbARQRpt = FALSE;

		if (intRepeatCount > 5)  // do 5 tries then force disconnect 
		{
			QueueCommandToHost("DISCONNECTED");
			sprintf(HostCmd, "STATUS END NOT RECEIVED CLOSING ARQ SESSION WITH %s", strRemoteCallsign);
			QueueCommandToHost(HostCmd);
			if (AccumulateStats) LogStats();

			blnDISCRepeating = FALSE;
			blnEnbARQRpt = FALSE;
			ClearDataToSend();
			SetARDOPProtocolState(DISC);
			intRepeatCount = 0;
			InitializeConnection();
			return FALSE;			 //indicates end repeat
		}
		WriteDebugLog(LOGDEBUG, "Repeating DISC %d", intRepeatCount);
		EncodeAndSend4FSKControl(DISCFRAME, bytSessionID, LeaderLength);

		return TRUE;			// continue with DISC repeats
	}

	if (ProtocolState == ISS || ProtocolState == IDLE)
		if (CheckForDisconnect())
			return FALSE;

	if (ProtocolState == ISS && ARQState == ISSConReq) // Handles Repeating ConReq frames 
	{
		intRepeatCount++;
		if (intRepeatCount > ARQConReqRepeats)
		{
		    ClearDataToSend();
			SetARDOPProtocolState(DISC);
			intRepeatCount = 0;
			blnPending = FALSE;
			displayCall(0x20, "");

			if (strRemoteCallsign[0])
			{
				sprintf(HostCmd, "STATUS CONNECT TO %s FAILED!", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				InitializeConnection();
				return FALSE;		// 'indicates end repeat
			}
			else
			{
				QueueCommandToHost("STATUS END ARQ CALL");
				InitializeConnection();
				return FALSE;		  //indicates end repeat
			}

			
			//Clear the mnuBusy status on the main form
            ///    Dim stcStatus As Status = Nothing
            //    stcStatus.ControlName = "mnuBusy"
            //    queTNCStatus.Enqueue(stcStatus)
		}

		return TRUE;		// ' continue with repeats
	}
	
	if (ProtocolState == ISS && ARQState == IRSConAck)
	{
		// Handles ISS repeat of ConAck

		intRepeatCount += 1;
		if (intRepeatCount <= ARQConReqRepeats)
			return TRUE;
		else
		{
			SetARDOPProtocolState(DISC);
			ARQState = DISCArqEnd;
			sprintf(HostCmd, "STATUS CONNECT TO %s FAILED!", strRemoteCallsign);
			QueueCommandToHost(HostCmd);
			intRepeatCount = 0;
			InitializeConnection();
			return FALSE;
		}
	}
	// Handles a timeout from an ARQ connected State

	if (ProtocolState == ISS || ProtocolState == IDLE || ProtocolState == IRS || ProtocolState == IRStoISS)
	{
		if ((Now - dttTimeoutTrip) / 1000 > ARQTimeout) // (Handles protocol rule 1.7)
		{
            if (!blnTimeoutTriggered)
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.GetNexARQFrame] Timeout setting SendTimeout timer to start.");

				blnEnbARQRpt = FALSE;
				blnTimeoutTriggered = TRUE; // prevents a retrigger
                tmrSendTimeout = Now + 1000;
				return FALSE;
			}
		}
	}

	if (ProtocolState == DISC && intPINGRepeats > 0)
	{
		intRepeatCount++;
		if (intRepeatCount <= intPINGRepeats && blnPINGrepeating)
		{
			dttLastPINGSent = Now;
			return TRUE;				// continue PING
		}
		
		intPINGRepeats = 0;
		blnPINGrepeating = False;
        return FALSE;

	}

	// Handles the DISC state (no repeats)
 
	if (ProtocolState == DISC) // never repeat in DISC state
	{
		blnARQDisconnect = FALSE;
		intRepeatCount = 0;
		return FALSE;
	}

	// ' Handles all other possibly repeated Frames

	return blnEnbARQRpt;  // not all frame types repeat...blnEnbARQRpt is set/cleared in ProcessRcvdARQFrame
}

 
// function to generate 8 bit session ID

UCHAR GenerateSessionID(char * strCallingCallSign, char *strTargetCallsign)
{
	char bytToCRC[20];
	
	int Len = sprintf(bytToCRC, "%s%s", strCallingCallSign, strTargetCallsign);

	UCHAR ID = GenCRC8(bytToCRC, strlen(bytToCRC));

	if ((ID & 0x3F) != 0x3F)
		ID &= 0x3F;
	else // rare case where the computed session ID woudl be 3F	
		ID = 0; //Remap a SessionID of 3F to 0...3F reserved for FEC mode
    
   	return ID;

}

// Function to compute the optimum leader based on the Leader sent and the reported Received leader
//	A3 doesn't report Received Leader, so can't do this

void xCalculateOptimumLeader(int intReportedReceivedLeaderMS,int  intLeaderSentMS)
{
	// intCalcLeader = max(200, 120 + intLeaderSentMS - intReportedReceivedLeaderMS);  //  This appears to work well on HF sim tests May 31, 2015
    //    WriteDebugLog(LOGDEBUG, ("[ARDOPprotocol.CalcualteOptimumLeader] Leader Sent=" & intLeaderSentMS.ToString & "  ReportedReceived=" & intReportedReceivedLeaderMS.ToString & "  Calculated=" & stcConnection.intCalcLeader.ToString)
}

 

// Function to determine if call is to Callsign or one of the AuxCalls

BOOL IsCallToMe(char * strCallsign)
{
	// returns true and sets bytReplySessionID if is to me.

	int i;
	
	if (strcmp(strCallsign, Callsign) == 0)
		return TRUE;
	
	for (i = 0; i < AuxCallsLength; i++)
	{
		if (strcmp(strCallsign, AuxCalls[i]) == 0)
			return TRUE;
	}

	return FALSE;
}
BOOL IsPingToMe(char * strCallsign)
{
	int i;
	
	if (strcmp(strCallsign, Callsign) == 0)
		return TRUE;
	
	for (i = 0; i < AuxCallsLength; i++)
	{
		if (strcmp(strCallsign, AuxCalls[i]) == 0)
			return TRUE;
	}

	return FALSE;
}
/*
ModeToSpeed() = {

	40 768
	42 
	44 1296
	46 429
	48
	4A 881
	4C
	4E 288

	50 1536
	52 2592
	54 
	56 4305
	58 429
	5A 329
	5C
	5E 

	60 3072
	62 5184
	64 
	66 8610
	68 1762
	6A
	6C
	6E 

	70 6144 
	72 10286
	74 
	76 17228
	78 3624
	7A 5863
	7C 4338
	7E 

	*/
// Function to get base (even) data modes by bandwidth for ARQ sessions

// 220 to 947 bytes/min

// Note lowest mode isn't normally used - it is a special short frame for short messages
// Most intervals are about 2x, but top is less, so shift down at higher average acks

static UCHAR DataModes200[] = {D4PSK_200_50_E, D4PSK_200_50_E, D4PSK_200_100_E, D4PSKR_200_100_E, D16APSK_200_100_E};
 
static int DataLengths200[] = {24, 24, 48, 72, 96};

static int upThresholds200[] = {75, 75, 75, 75, 75};
static int downThresholds200[] = {40, 40, 40, 40, 55};	// may need tuning

// 220 to 1895 bytes/min
static UCHAR DataModes500[] = {D4PSK_500_100S_E, D4PSK_200_50_E, D4PSK_500_50_E, D4PSK_500_100_E, D4PSKR_500_100_E, D16APSK_500_100_E};

static int DataLengths500[] = {12, 24, 48, 96, 144, 192};

static int upThresholds500[] = {75, 75, 75, 75, 75, 75};
static int downThresholds500[] = {40, 40, 40, 40, 40, 55};

// 439 to 9921 bytes/min
static UCHAR DataModes2500[] = {D4PSK_500_100S_E, D4PSK_500_50_E, D4PSK_500_100_E, D4PSK_1000_100_E,
								D4PSK_2500_100_E, D4PSKC_2500_200_E, D4PSKCR_2500_200_E};
static int DataLengths2500[] = {12, 48, 96, 192, 480, 800, 1100};

static int upThresholds2500[] = {75, 75, 75, 75, 75, 75, 75};
static int downThresholds2500[] = {40, 40, 40, 40, 40, 40, 55};

static UCHAR NoDataModes[1] = {0};

UCHAR  * GetDataModes(int intBW)
{
	// Revised version 0.3.5
	// idea is to use this list in the gear shift algorithm to select modulation mode based on bandwidth and robustness.
    // Sequence modes in approximate order of robustness ...most robust first, shorter frames of same modulation first

	if (intBW == 200)
	{
		bytFrameTypesForBWLength = sizeof(DataModes200);
		bytFrameLengthsForBW = DataLengths200;
		upThresholds = upThresholds200;
		downThresholds = downThresholds200;
		return DataModes200;
	}
	if (intBW == 500) 
	{
		bytFrameTypesForBWLength = sizeof(DataModes500);
		bytFrameLengthsForBW = DataLengths500;
		upThresholds = upThresholds500;
		downThresholds = downThresholds500;
		return DataModes500;
	}

	if (intBW == 2500) 
	{	
		bytFrameTypesForBWLength = sizeof(DataModes2500);
		bytFrameLengthsForBW = DataLengths2500;
		upThresholds = upThresholds2500;
		downThresholds = downThresholds2500;
		return DataModes2500;
	}
	bytFrameTypesForBWLength = 0;
	return NoDataModes;
}

unsigned short  ModeHasWorked[16] = {0};		// used to attempt to make gear shift more stable.
unsigned short  ModeHasBeenTried[16] = {0};
unsigned short  ModeNAKS[16] = {0};
unsigned short  CarrierACKS[16] = {0};
unsigned short  CarrierNAKS[16] = {0};

//  Subroutine to shift up to the next higher throughput or down to the next more robust data modes based on average reported quality 

int delayShiftUp = 0;

extern float RollingAverage;

VOID Gearshift(float AckPercent, BOOL SomeAcked)
{
	char strOldMode[18] = "";
	char strNewMode[18] = "";

	int intBytesRemaining = bytDataToSendLength;

	int upThreshold = upThresholds[intFrameTypePtr];
	int downThreshold = downThresholds[intFrameTypePtr];

	intShiftUpDn = 0;

	// At start of session shift or down up quicker

	if (AckPercent >= 60.0 && ModeHasBeenTried[intFrameTypePtr + 1] == 0)		// First pass
		AckPercent =  80.0f;

	if (AckPercent > upThreshold)
	{		
		// if the next new mode has been tried before, and immediately failed, don't try again
		// till we get at least 5 sucessive acks

		if (ModeHasBeenTried[intFrameTypePtr + 1] && ModeHasWorked[intFrameTypePtr + 1] == 0)
		{
			delayShiftUp ++;
			if (delayShiftUp < 3)
			{
				WriteDebugLog(LOGINFO, "[Gearshift]  Delaying Shift to unsuccessful mode: AckPercent = %f", AckPercent);
				return;
			}
			// ok to try to shift up again
		}

		if (intFrameTypePtr < (bytFrameTypesForBWLength - 1))
		{
			// Can shift up

	 		strcpy(strOldMode, Name(bytFrameTypesForBW[intFrameTypePtr]));
			strOldMode[strlen(strOldMode) - 2] = 0;
			strcpy(strNewMode, Name(bytFrameTypesForBW[intFrameTypePtr + 1]));
			strNewMode[strlen(strNewMode) - 2] = 0;
			WriteDebugLog(LOGINFO, "[Gearshift]  Shift Up: AckPercent = %f Shift up from Frame type %s New Mode: %s", AckPercent, strOldMode, strNewMode);
			intShiftUpDn = 1;
			RollingAverage = 50;		// preset to nominal middle on shift
			intShiftUPs++;
			delayShiftUp = 0;

			ModeHasBeenTried[intFrameTypePtr + intShiftUpDn] = 1;
		}
		else
		{
			// Hi quality, but no more modes.
			// Limit Quality, so we can go back down without excessive retries

			RollingAverage = 75.0f;
			WriteDebugLog(LOGINFO, "[Gearshift]  No Change possible: AckPercent = %f", AckPercent);
		}

		return;
	}

	if (AckPercent <= downThreshold) 
	{
		if (intFrameTypePtr > 1)		// zero not used
		{
			// Can shift down

			strcpy(strOldMode, Name(bytFrameTypesForBW[intFrameTypePtr]));
			strOldMode[strlen(strOldMode) - 2] = 0;	// Remove .E
			strcpy(strNewMode, Name(bytFrameTypesForBW[intFrameTypePtr - 1]));
			strNewMode[strlen(strNewMode) - 2] = 0;
	
			WriteDebugLog(LOGINFO, "[Gearshift]  Shift Down: AckPercent = %f Shift down from Frame type %s New Mode: %s", AckPercent, strOldMode, strNewMode);
			intShiftUpDn = -1;
			intShiftDNs++;
			RollingAverage = 50.0f;		// preset to nominal middle on shift
			ModeHasBeenTried[intFrameTypePtr + intShiftUpDn] = 1;
		}
		else
		{
			// Low quality, but no more modes.
			// Limit Quality, so we can go back up without excessive retries

			RollingAverage = 40.0f;
			WriteDebugLog(LOGINFO, "[Gearshift]  No Change possible: AckPercent = %f", AckPercent);
		}
		return;
	}
	// No Shift
		
	WriteDebugLog(LOGINFO, "[Gearshift]  No Change: AckPercent = %f", AckPercent);
}

int GetNumCarriers(UCHAR bytFrameType)
{
	int intNumCar, dummy;
	char strMod[16];

	if (FrameInfo(bytFrameType, &dummy, &intNumCar, strMod, &dummy, &dummy, &dummy, &dummy))
		return intNumCar;
	
	return 0;
}
 
 // Subroutine to determine the next data frame to send (or IDLE if none) 

void SendData()
{
	char strMod[16];
	int Len;

	// Check for ID frame required (every 10 minutes)
	
	if (blnDISCRepeating)
		return;
	
	switch (ProtocolState)
	{

	// I now actively repeat IDLE instead of using timeout to resend

	case IDLE:

		blnEnbARQRpt = TRUE;			
		blnLastFrameSentData = FALSE;

		// This is the repeat after timeout time, not the normal interval after an ack

		intFrameRepeatInterval = ComputeInterFrameInterval(3000);  // keep IDLE repeats at 2 sec 
	
		WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.SendData]  Continue sending  IDLE");
	
		// Initial Idle repeat interval is short to speed up turnround
		// Start delaying when we repeat.

		txSleep(100);

		EncodeAndSend4FSKControl(IDLEFRAME, bytSessionID, LeaderLength); // only returns when all sent
  		return;


	case ISS:
			
		if (CheckForDisconnect())
			return;
		
		Send10MinID();  // Send ID if 10 minutes since last

		// Drop through

	case IRStoISS:

		if (unackedByteCount > 0 || bytDataToSendLength > 0)
		{
			WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.SendData] DataToSend = %d bytes, In ProtocolState ISS", bytDataToSendLength);

			//' Get the data from the buffer here based on current data frame type
			//' (Handles protocol Rule 2.1)

			// Actually all GetNextFrameData does is to decide what mode to send next

			Len = bytQDataInProcessLen = GetNextFrameData(&intShiftUpDn, &bytCurrentFrameType, strMod, FALSE);

			blnLastFrameSentData = TRUE;

			// This mechanism lengthens the intFrameRepeatInterval for multiple carriers (to provide additional decoding time at remote end)
			// This does not slow down the throughput significantly since if an ACK or NAK is received by the sending station 
			// the repeat interval does not come into play.

			switch(GetNumCarriers(bytCurrentFrameType))
			{
			case 1:
                intFrameRepeatInterval = ComputeInterFrameInterval(1500); // fairly conservative based on measured leader from remote end 
				break;

			case 2:
				intFrameRepeatInterval = ComputeInterFrameInterval(1700); //  fairly conservative based on measured leader from remote end 
				break;

			case 4:                
				intFrameRepeatInterval = ComputeInterFrameInterval(1900); // fairly conservative based on measured leader from remote end 
				break;
			
			case 10:
                intFrameRepeatInterval = ComputeInterFrameInterval(2100); // fairly conservative based on measured leader from remote end 
                break;

			default:
				intFrameRepeatInterval = 2000;  // shouldn't get here
			}

			dttTimeoutTrip = Now;
			blnEnbARQRpt = TRUE;
			ARQState = ISSData;		 // Should not be necessary

			EncodeData(bytCurrentFrameType);
			ModCarrierSet(intCalcLeader);
	
			return;
		}
		else
		{
			// Nothing to send - set IDLE

			SetARDOPProtocolState(IDLE);

			blnEnbARQRpt = TRUE;
			dttTimeoutTrip = Now;
			
			blnLastFrameSentData = FALSE;

			// This is the repeat after timeout time, not the normal interval after an ack

			intFrameRepeatInterval = ComputeInterFrameInterval(2000);  // keep IDLE repeats at 2 sec 

			ClearDataToSend(); // ' 0.6.4.2 This insures new OUTOUND queue is updated (to value = 0)
	
			WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.SendData]  Send IDLE, Set ProtocolState=IDLE ");
	
			EncodeAndSend4FSKControl(IDLEFRAME, bytSessionID, LeaderLength); // only returns when all sent
  			return;
		}
	}
}

int GetTempMode(int RealMode)
{
	// find lowest mode to sent current outstanding data

	int TempMode = -1;
	{
		int i = 0;

		while (i < RealMode)		// All modes slower than current
		{
			if (bytDataToSendLength <= bytFrameLengthsForBW[i])
			{
				TempMode = i;	
				break;
			}
			i++;
		}
	}
	return TempMode;
}

//	a simple function to get an available frame type for the session bandwidth. 
    
int	TempModeRetries = 0;
	
int GetNextFrameData(int * intUpDn, UCHAR * bytFrameTypeToSend, UCHAR * strMod, BOOL blnInitialize)
{
	// Initialize if blnInitialize = true
	// Then call with intUpDn and blnInitialize = FALSE:
	//       intUpDn = 0 ' use the current mode pointed to by intFrameTypePtr
	//       intUpdn < 0    ' Go to a more robust mode if available limited to the most robust mode for the bandwidth 
	//       intUpDn > 0    ' Go to a less robust (faster) mode if avaialble, limited to the fastest mode for the bandwidth

	BOOL blnOdd;
	int intNumCar, intBaud, intDataLen, intRSLen;
    char * strShift = "No shift";

	int MaxLen, totSymbols;

	if (blnInitialize)	//' Get the array of supported frame types in order of Most robust to least robust
	{
		bytFrameTypesForBW = GetDataModes(intSessionBW);

		GetNAKLoQLevels(intSessionBW);
		GetACKHiQLevels(intSessionBW);
 
		if (fastStart)
			intFrameTypePtr = ((bytFrameTypesForBWLength - 1) >> 1);	// Start mid way
		else
			intFrameTypePtr = 1;	// zero not used

		bytCurrentFrameType = bytFrameTypesForBW[intFrameTypePtr];

		ResetTXState();
		TempMode = -1;
		ModeHasBeenTried[intFrameTypePtr] = 1;

#ifdef PLOTCONSTELLATION
		DrawTXMode(shortName(bytCurrentFrameType));
		updateDisplay();
#endif
		if(DebugLog) WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.GetNextFrameData] Initial Frame Type: %s", Name(bytCurrentFrameType));

		blnFramePending = False;
		*intUpDn = 0;
		return 0;
	}

	if (TempMode != -1)
	{
		// Shift back to original mode

		// We must only requeue data if we are changing mode, but how??

		// I think the only safe way is once we send a Temp Mode we stay in it until all acked

		// But then doesn't shift down, as temp mode acks don't
		// pass through gearshift. This prevents initial shift down
		// if first mode too high

		// So if too many repeats of temp mode, shift to mode below temp mode.

		if (unackedByteCount == 0)
		{
			// All acked so go back to original mode
			// But if current data will fit in current temp mode
			// stay in it

			if (GetTempMode(SavedMode) == TempMode)
			{
				WriteDebugLog(LOGDEBUG, "[GetNextFrameData] Data will fit in current Temp Mode - stay in it");

				if ((bytCurrentFrameType & 1) == (bytLastARQDataFrameAcked & 1))
				{
					*bytFrameTypeToSend = bytCurrentFrameType ^ 1;  // This ensures toggle of  Odd and Even 
					bytLastARQDataFrameSent = *bytFrameTypeToSend;
				}
				return 0;		// Cant have pending shift after ack of temp mode
			}

			intFrameTypePtr = SavedMode;
			TempMode = -1;
			bytCurrentFrameType = bytFrameTypesForBW[intFrameTypePtr];

#ifdef PLOTCONSTELLATION
			DrawTXMode(shortName(bytCurrentFrameType));
			updateDisplay();
#endif
			strShift = "Shift back after temp shift down";
	
			goto tryTempMode;		// Cant have pending shift after ack of temp mode
		}
		
		TempModeRetries++;
			
		if (TempModeRetries < 2 || intFrameTypePtr < 2) // Make sure can shift
		{
			WriteDebugLog(LOGDEBUG, "[GetNextFrameData] Not all Data Acked - stay in Temp Mode");
			return 0;
		}

		//  shift down (relative to temp mode)
	
		*intUpDn = -1;
		TempModeRetries = 0;
	
		WriteDebugLog(LOGDEBUG, "[GetNextFrameData] Shifting down from Temp Mode");
	}

	if (*intUpDn < 0)		// go to a more robust mode
	{
		if (intFrameTypePtr > 0)
		{
			intFrameTypePtr = max(1, intFrameTypePtr + *intUpDn);
			bytCurrentFrameType = bytFrameTypesForBW[intFrameTypePtr];
			RequeueData();

#ifdef PLOTCONSTELLATION
			DrawTXMode(shortName(bytCurrentFrameType));
			updateDisplay();
#endif
			strShift = "Shift Down";
		}
		*intUpDn = 0;
	}
	else if (*intUpDn > 0)	//' go to a faster mode
	{
		if (intFrameTypePtr < bytFrameTypesForBWLength)
		{
			intFrameTypePtr = min(bytFrameTypesForBWLength, intFrameTypePtr + *intUpDn);
			bytCurrentFrameType = bytFrameTypesForBW[intFrameTypePtr];
			RequeueData();

#ifdef PLOTCONSTELLATION
			DrawTXMode(shortName(bytCurrentFrameType));
			updateDisplay();
#endif
			strShift = "Shift Up";
		}
		*intUpDn = 0;
	}

	// If no data is outstanding and remaining data can be sent in a more
	// robust mode, temporarily shift to it

tryTempMode:

	TempMode = -1;

	if (unackedByteCount == 0)
	{
		int i = 0;

		while (i < intFrameTypePtr)		// All modes slower than current
		{
			if (bytDataToSendLength <= bytFrameLengthsForBW[i])
			{
				TempMode = i;	
				break;
			}
			i++;
		}
	}

	if (TempMode != -1)
	{
		// Shift to it

		TempModeRetries = 0;
		SavedMode =	intFrameTypePtr;

		intFrameTypePtr = TempMode;
		bytCurrentFrameType = bytFrameTypesForBW[intFrameTypePtr];

		// no need for RequeueData() as only called if none outstanding;

#ifdef PLOTCONSTELLATION
		DrawTXMode(shortName(bytCurrentFrameType));
		updateDisplay();
#endif
		strShift = "Temporary Shift Down";
	}

	// if Type has changed, reset TX PSN
		
	if ((bytCurrentFrameType & 0xfe) != (bytLastARQDataFrameAcked & 0xfe))
	{
		ResetTXState(0);

		// I think we should also requeue any outstanding data. This traps a change of TempMode.

		RequeueData();
	}

	// Make sure Toggle is maintained on type change

	if ((bytCurrentFrameType & 1) == (bytLastARQDataFrameAcked & 1))
	{
		*bytFrameTypeToSend = bytCurrentFrameType ^ 1;  // This ensures toggle of  Odd and Even 
		bytLastARQDataFrameSent = *bytFrameTypeToSend;
	}
	else
	{
		*bytFrameTypeToSend = bytCurrentFrameType;
		bytLastARQDataFrameSent = *bytFrameTypeToSend;
	}
	
	if (DebugLog)
	{
		WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.GetNextFrameData] %s, Frame Type: %s", strShift, Name(bytCurrentFrameType));
	}

	FrameInfo(bytCurrentFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &totSymbols);

	MaxLen = intDataLen * intNumCar;

	if (MaxLen > bytDataToSendLength)
		MaxLen = bytDataToSendLength;

	return MaxLen;
}
 

void InitializeConnection()
{
	// Sub to Initialize before a new Connection

	strRemoteCallsign[0] = 0; // remote station call sign
	intOBBytesToConfirm = 0; // remaining bytes to confirm  
	intBytesConfirmed = 0; // Outbound bytes confirmed by ACK and squenced
	intReceivedLeaderLen = 0; // Zero out received leader length (the length of the leader as received by the local station
	intReportedLeaderLen = 0; // Zero out the Reported leader length the length reported to the remote station 
	bytSessionID = 0x3F; //  Session ID 
	blnLastPSNPassed = FALSE; //  the last PSN passed True for Odd, FALSE for even. 
	blnInitiatedConnection = FALSE; //  flag to indicate if this station initiated the connection
	dblAvgPECreepPerCarrier = 0; //  computed phase error creep
	dttLastIDSent = Now ; //  date/time of last ID
	intTotalSymbols = 0; //  To compute the sample rate error
	strLocalCallsign[0] = 0; //  this stations call sign
	intSessionBW = 0; 
	bytLastACKedDataFrameType = 0;
	bytCurrentFrameType = 0;

	intCalcLeader = LeaderLength;

	ClearQualityStats();
	ClearTuningStats();

	memset(ModeHasWorked, 0, sizeof(ModeHasWorked));
	memset(ModeHasBeenTried, 0, sizeof(ModeHasBeenTried));
	memset(ModeNAKS, 0, sizeof(ModeNAKS));
	memset(CarrierACKS, 0, sizeof(CarrierACKS));
	memset(CarrierNAKS, 0, sizeof(CarrierNAKS));

	ResetPSNState();
}

// This sub processes a correctly decoded ConReq frame, decodes it an passed to host for display if it doesn't duplicate the prior passed frame. 

void ProcessUnconnectedConReqFrame(int intFrameType, UCHAR * bytData)
{
	char strDisplay[128];
	int Len;

	if (!(intFrameType >= ConReq200 && intFrameType <= ConReq2500))
		return;

	//" [ConReq2500 >  G8XXX] "
 
	Len = sprintf(strDisplay, " [%s > %s]", Name(intFrameType), bytData); 
	if (strcmp(strLastStringPassedToHost, strDisplay) == 0)
		return;

	AddTagToDataAndSendToHost(strDisplay, "ARQ", Len);
}

 
//	This is the main subroutine for processing ARQ frames 

void ProcessRcvdARQFrame(UCHAR intFrameType, UCHAR * bytData, int DataLen, BOOL blnFrameDecodedOK)
{
	//	blnFrameDecodedOK should always be true except in the case of a failed data frame ...Which is then NAK'ed if in IRS Data state
    
	int intReply;
	static UCHAR * strCallsign;
	int intReportedLeaderMS = 0;
	char HostCmd[80];
	int timeSinceDecoded = Now - DecodeCompleteTime;

	// Allow for link turnround before responding

	WriteDebugLog(LOGDEBUG, "Time since received = %d", timeSinceDecoded);

	if (timeSinceDecoded < 250)
		txSleep(250 - timeSinceDecoded);

	// Note this is called as part of the RX sample poll routine

	switch (ProtocolState)
	{
	case DISC:
		
		// DISC State *******************************************************************************************

		if (blnFrameDecodedOK && intFrameType == DISCFRAME) 
		{
			// Special case to process DISC from previous connection (Ending station must have missed END reply to DISC) Handles protocol rule 1.5
    
			WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received in ProtocolState DISC, Send END with SessionID= %X Stay in DISC state", bytLastARQSessionID);

			tmrFinalID = Now + 3000;			
			blnEnbARQRpt = FALSE;
			EncodeAndSend4FSKControl(END, bytLastARQSessionID, LeaderLength);

			return;
		}

		if (intFrameType == PING && blnFrameDecodedOK)
		{
			ProcessPingFrame(bytData);
            return;
		}

		if (intFrameType == CQ_de && blnFrameDecodedOK)
		{
			ProcessCQFrame(bytData);
            return;
		}
    
		// Process Connect request to MyCallsign or Aux Call signs  (Handles protocol rule 1.2)
   
		if (!blnFrameDecodedOK || intFrameType < ConReq200 || intFrameType > ConReq2500)
			return;			// No decode or not a ConReq

		strlop(bytData, ' ');	 // Now Just Tocall
		strCallsign  =  bytData;

		WriteDebugLog(LOGDEBUG, "CONREQ to %s Listen = %d", strCallsign, blnListen);

		if (!blnListen)
			return;			 // ignore connect request if not blnListen

		// see if connect request is to MyCallsign or any Aux call sign
        
		if (IsCallToMe(strCallsign)) // (Handles protocol rules 1.2, 1.3)
		{
			BOOL blnLeaderTrippedBusy;
			
			// This logic works like this: 
			// The Actual leader for this received frame should have tripped the busy detector making the last Busy trip very close
			// (usually within 100 ms) of the leader detect time. So the following requires that there be a Busy clear (last busy clear) following 
			// the Prior busy Trip AND at least 600 ms of clear time (may need adjustment) prior to the Leader detect and the Last Busy Clear
			// after the Prior Busy Trip. The initialization of times on objBusy.ClearBusy should allow for passing the following test IF there
			// was no Busy detection after the last clear and before the actual reception of the next frame. 

			blnLeaderTrippedBusy = (dttLastLeaderDetect - dttLastBusyTrip) < 300;
	
			if (BusyBlock)
			{
				if ((blnLeaderTrippedBusy && dttLastBusyClear - dttPriorLastBusyTrip < 600) 
				|| (!blnLeaderTrippedBusy && dttLastBusyClear - dttLastBusyTrip < 600))
				{
					WriteDebugLog(LOGDEBUG, "[ProcessRcvdARQFrame] Con Req Blocked by BUSY!  LeaderTrippedBusy=%d, Prior Last Busy Trip=%d, Last Busy Clear=%d,  Last Leader Detect=%d",
						blnLeaderTrippedBusy, Now - dttPriorLastBusyTrip, Now - dttLastBusyClear, Now - dttLastLeaderDetect);

					ClearBusy();
			
					// Clear out the busy detector. This necessary to keep the received frame and hold time from causing
					// a continuous busy condition.

 					EncodeAndSend4FSKControl(ConRejBusy, bytPendingSessionID, LeaderLength);

					sprintf(HostCmd, "REJECTEDBUSY %s", strRemoteCallsign);
					QueueCommandToHost(HostCmd);
					sprintf(HostCmd, "STATUS ARQ CONNECTION REQUEST FROM %s REJECTED, CHANNEL BUSY.", strRemoteCallsign);
					QueueCommandToHost(HostCmd);

					return;
				}
			}

			InitializeConnection();	

			intReply = IRSNegotiateBW(intFrameType); // NegotiateBandwidth

			if (intReply != ConRejBW)	// If not ConRejBW the bandwidth is compatible so answer with correct ConAck frame
			{
				GetNAKLoQLevels(intSessionBW);
				GetACKHiQLevels(intSessionBW);
 
				sprintf(HostCmd, "TARGET %s", strCallsign);
				QueueCommandToHost(HostCmd);

				bytDataToSendLength = 0;		// Clear queue

				SetLED(TRAFFICLED, FALSE);

				blnPending = TRUE;				
				blnEnbARQRpt = FALSE;

				//Timeout increased as handshake is now longer 30 secs may be too much

				tmrIRSPendingTimeout = Now + 30000;  // Triggers a 10 second timeout before auto abort from pending

				// (Handles protocol rule 1.2)
                            
				dttTimeoutTrip = Now;
                            
				SetARDOPProtocolState(IRS);
				ARQState = IRSConAck; // now connected 

				intLastARQDataFrameToHost = -1;	 // precondition to an illegal frame type
 				memset(CarrierOk, 0, sizeof(CarrierOk));	// CLear MEM ARQ Stuff
				LastDataFrameType = -1;
  
				strcpy(strRemoteCallsign, bytData);
				strcpy(strLocalCallsign, strCallsign);
				strcpy(strFinalIDCallsign, strCallsign);

				intReceivedLeaderLen = intLeaderRcvdMs;		 // capture the received leader from the remote ISS's ConReq (used for timing optimization)
				dttLastFECIDSent = Now;
//				EncodeAndSend4FSKControl(ConAck, bytPendingSessionID, 200);
				EncodeAndSend4FSKControl(ConAck, bytPendingSessionID, LeaderLength);
			}
			else
			{
				// ' ConRejBW  (Incompatible bandwidths)

				// ' (Handles protocol rule 1.3)
             
				sprintf(HostCmd, "REJECTEDBW %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
			
				EncodeAndSend4FSKControl(intReply, bytPendingSessionID, LeaderLength);
			}
		}
		else
		{
			// Not for us - cancel pending
			
			QueueCommandToHost("CANCELPENDING");
//			ProcessUnconnectedConReqFrame(intFrameType, bytData);  //  displays data if not connnected.  
		}
		blnEnbARQRpt = FALSE;
		return;

		// IDLE must be just before IRS as we may need to drop through

		case IDLE:			// The state where the ISS has no data to send and is looking for a BREAK from the IRS
 
			if (!blnFrameDecodedOK)
				return; // No decode so continue to wait

			// I don't think IDLE should just repeat as I want
			// to have different timout and repeat intervals.
			// So on ACK cancel and repeat and resend IDLE if nothing
			// to send else send data

			if (intFrameType == ACK)
			{
				if (bytDataToSendLength > 0)	 // If ACK and Data to send
				{
					WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessedRcvdARQFrame] Protocol state IDLE, ACK Received with Data to send. Go to ISS Data state.");
					
					SetARDOPProtocolState(ISS);
					ARQState = ISSData;
					SendData(FALSE);
					return;
				}
				else
				{
					// ACK with no data.

					SetARDOPProtocolState(IDLE);
					ARQState = ISSData;
					SendData(FALSE);		// Will send IDLE as no data
					return;
				}
			}

			// process BREAK here Send ID if over 10 min. 

			if (intFrameType == BREAK)
			{
				// Initiate the transisiton to IRS

				dttTimeoutTrip = Now;
				blnEnbARQRpt = FALSE;
				EncodeAndSend4FSKControl(OVER, bytSessionID, LeaderLength);

				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd from IDLE, Go to IRS, Substate IRSfromISS");
                SendCommandToHost("STATUS BREAK received from Protocol State IDLE, new state IRS");
				SetARDOPProtocolState(IRS);
				//Substate IRSfromISS enables processing Rule 3.5 later
				ARQState = IRSfromISS; 
				
				intLinkTurnovers += 1;
				intLastARQDataFrameToHost = -1;  // precondition to an illegal frame type (ensures the new IRS does not reject a frame)
				memset(CarrierOk, 0, sizeof(CarrierOk));	// CLear MEM ARQ Stuff
				LastDataFrameType = -1;
				return;
			}
			if (intFrameType == DISCFRAME) //  IF DISC received from IRS Handles protocol rule 1.5
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received Send END...go to DISC state");
                
				if (AccumulateStats) LogStats();
					
				QueueCommandToHost("DISCONNECTED");
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s ", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				tmrFinalID = Now + 3000;
				blnDISCRepeating = FALSE;
				EncodeAndSend4FSKControl(END, bytSessionID, LeaderLength);
                bytLastARQSessionID = bytSessionID; // capture this session ID to allow answering DISC from DISC state
                ClearDataToSend();
                SetARDOPProtocolState(DISC);
				blnEnbARQRpt = FALSE;
				return;
			}
			if (intFrameType == END)
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  END received ... going to DISC state");
				if (AccumulateStats) LogStats();
				QueueCommandToHost("DISCONNECTED");	
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s ", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				ClearDataToSend();

				if (CheckValidCallsignSyntax(strLocalCallsign))
				{
					dttLastFECIDSent = Now;
					EncLen = EncodePSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes, 0x3F);
					Mod1Car50Bd4PSK(&bytEncodedBytes[0], EncLen, 0);		// only returns when all sent
				}     
				SetARDOPProtocolState(DISC); 
				blnEnbARQRpt = FALSE;
				blnDISCRepeating = FALSE;
				return;
			}
			
			if (IsDataFrame(intFrameType))
			{
				// Transition to IRS and drop through to process

				dttTimeoutTrip = Now;
				blnEnbARQRpt = FALSE;

				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] Data Rcvd from IDLE, Go to IRS, Substate IRSfromISS");
                SendCommandToHost("STATUS BREAK received from Protocol State IDLE, new state IRS");
				SetARDOPProtocolState(IRS);
				//Substate IRSfromISS enables processing Rule 3.5 later
				ARQState = IRSfromISS; 
				
				intLinkTurnovers += 1;

				// We already have the first frame. so mustn't reset LastDataFrameType.
				// We haven't passed anything to host so can set intLastARQDataFrameToHost

				intLastARQDataFrameToHost = -1;  // precondition to an illegal frame type (ensures the new IRS does not reject a frame)
//				LastDataFrameType = -1;
			}
			else

				// Shouldn'r get here ??
  				return;
		

	

	case IRS:

		//IRS State ****************************************************************************************
		//  Process ConReq, ConAck, DISC, END, Host DISCONNECT, DATA, IDLE, BREAK 

		if (ARQState == IRSConAck)		// Process ConAck or ConReq if reply ConAck sent above in Case ProtocolState.DISC was missed by ISS
		{         
			if (!blnFrameDecodedOK)
				return;					// no reply if no correct decode

			// ConReq processing (to handle case of ISS missing initial ConAck from IRS)

			if (intFrameType >= ConReq200 && intFrameType <= ConReq2500) // Process Connect request to MyCallsign or Aux Call signs as for DISC state above (ISS must have missed initial ConACK from ProtocolState.DISC state)
			{
				if (!blnListen)
					return;
				
				// see if connect request is to MyCallsign or any Aux call sign

				strlop(bytData, ' ');	 // Now Just Tocall
				strCallsign  =  bytData;
       
				if (IsCallToMe(strCallsign)) // (Handles protocol rules 1.2, 1.3)
				{
					//WriteDebugLog(LOGDEBUG, "[ProcessRcvdARQFrame]1 strCallsigns(0)=" & strCallsigns(0) & "  strCallsigns(1)=" & strCallsigns(1) & "  bytPendingSessionID=" & Format(bytPendingSessionID, "X"))
            
					InitializeConnection();
					intReply = IRSNegotiateBW(intFrameType); // NegotiateBandwidth

					if (intReply != ConRejBW)	// If not ConRejBW the bandwidth is compatible so answer with correct ConAck frame
					{
						// Note: CONNECTION and STATUS notices were already sent from  Case ProtocolState.DISC above...no need to duplicate

  						SetARDOPProtocolState(IRS);
						ARQState = IRSConAck; // now connected 

						intLastARQDataFrameToHost = -1;	 // precondition to an illegal frame type
						memset(CarrierOk, 0, sizeof(CarrierOk));	// CLear MEM ARQ Stuff
						LastDataFrameType = -1;
  
						intReceivedLeaderLen = intLeaderRcvdMs;		 // capture the received leader from the remote ISS's ConReq (used for timing optimization)
						bytDataToSendLength = 0;

						dttTimeoutTrip = Now;

						//Stop and restart the Pending timer upon each ConReq received to ME
 						tmrIRSPendingTimeout = Now + 30000;  // Triggers a 30 second timeout before auto abort from pending

						strcpy(strRemoteCallsign, bytData);
						strcpy(strLocalCallsign, strCallsign);
						strcpy(strFinalIDCallsign, strCallsign);

						EncodeAndSend4FSKControl(ConAck, bytPendingSessionID, LeaderLength);
						// ' No delay to allow ISS to measure its TX>RX delay}
						return;
					}
		
					// ' ConRejBW  (Incompatible bandwidths)

					// ' (Handles protocol rule 1.3)
             
					//	WriteDebugLog(LOGDEBUG, ("[ProcessRcvdARQFrame] Incompatible bandwidth connect request. Frame type: " & objFrameInfo.Name(intFrameType) & "   MCB.ARQBandwidth:  " & MCB.ARQBandwidth)
  					
					sprintf(HostCmd, "REJECTEDBW %s", strRemoteCallsign);
					QueueCommandToHost(HostCmd);
					sprintf(HostCmd, "STATUS ARQ CONNECTION FROM %s REJECTED, INCOMPATIBLE BANDWIDTHS.", strRemoteCallsign);
					QueueCommandToHost(HostCmd);

					EncodeAndSend4FSKControl(intReply, bytPendingSessionID, LeaderLength);

					return;
				}

				//this normally shouldn't happen but is put here in case another Connect request to a different station also on freq...may want to change or eliminate this
				
				//if (DebugLog) WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] Call to another target while in ProtocolState.IRS, ARQSubStates.IRSConAck...Ignore");

				return;
			}

			if (intFrameType == IDFRAME)
			{
				char * Loc = strlop(&bytData[3], ' ');
				char Msg[80];

				strcpy(strRemoteCallsign, &bytData[3]);
				strcpy(strGridSquare, &Loc[1]);
				strlop(strGridSquare, ']');

				displayCall('<', strRemoteCallsign);


				sprintf(Msg, "CONNECTED %s %d [%s]", strRemoteCallsign, intSessionBW, strGridSquare);
				SendCommandToHost(Msg);

				
				sprintf(Msg, "STATUS ARQ CONNECTION ESTABLISHED WITH %s,BW=%d,GS=%s", strRemoteCallsign, intSessionBW, strGridSquare);
				SendCommandToHost(Msg);
				ProtocolState = IRS;
				ARQState = IRSData;		//  Now connected 
				tmrIRSPendingTimeout = 0;
				blnPending = False;
				blnARQConnected = True;
				blnEnbARQRpt = False;	// 'setup for no repeats 
				bytSessionID = bytPendingSessionID; // This sets the session ID now 
				bytSessionID = GenerateSessionID(strRemoteCallsign, strLocalCallsign);

				// As we now send data in response to IDLE do we need to BREAK??

//				SetARDOPProtocolState(IRStoISS); // (ONLY IRS State where repeats are used)
//				EncodeAndSend4FSKControl(BREAK, bytSessionID, LeaderLength);

				EncodeAndSend4FSKControl(ACK, bytSessionID, LeaderLength);

				return;		

			}

		}

		if (ARQState == IRSData || ARQState == IRSfromISS)  // Process Data or ConAck if ISS failed to receive ACK confirming bandwidth so ISS repeated ConAck
		{
			if (blnFrameDecodedOK && intFrameType == IDFRAME)
			{
				//  Process ID frames (ISS failed to receive prior ACK)

				// But beware of 10 Minute ID, which shouldnt be acked

				if ((Now - dttStartSession) > 300000)		// 5 mins - if connection takes that long something is wrong!
					return;
      
				dttTimeoutTrip = Now;

				EncodeAndSend4FSKControl(ACK, bytSessionID, LeaderLength);
				return;
			}

			// handles DISC from ISS

			if (blnFrameDecodedOK && intFrameType == DISCFRAME) //  IF DISC received from ISS Handles protocol rule 1.5
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received in ProtocolState IRS, IRSData...going to DISC state");
                if (AccumulateStats) LogStats();
				
				QueueCommandToHost("DISCONNECTED");		// Send END
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				
				bytLastARQSessionID = bytSessionID;  // capture this session ID to allow answering DISC from DISC state if ISS missed Sent END
                       
				ClearDataToSend();
				tmrFinalID = Now + 3000;
				blnDISCRepeating = FALSE;
				
				SetARDOPProtocolState(DISC);
                InitializeConnection();
				blnEnbARQRpt = FALSE;

				EncodeAndSend4FSKControl(END, bytSessionID, LeaderLength);
				return;
			}
			
			// handles END from ISS
			
			if (blnFrameDecodedOK && intFrameType == END) //  IF END received from ISS 
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  END frame received in ProtocolState IRS, IRSData...going to DISC state");
				if (AccumulateStats) LogStats();
				
				QueueCommandToHost("DISCONNECTED");
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				blnDISCRepeating = FALSE;
				ClearDataToSend();

				SetARDOPProtocolState(DISC);
				blnEnbARQRpt = FALSE;

				if (CheckValidCallsignSyntax(strLocalCallsign))
				{
					dttLastFECIDSent = Now;
					EncLen = EncodePSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes, 0x3F);
					Mod1Car50Bd4PSK(&bytEncodedBytes[0], EncLen, 0);		// only returns when all sent
				}

				InitializeConnection();
				

				return;
			}

			// handles BREAK from remote IRS that failed to receive ACK
			
			if (blnFrameDecodedOK && intFrameType == BREAK)
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  BREAK received in ProtocolState %s , IRSData. Sending ACK", ARDOPStates[ProtocolState]);

				blnEnbARQRpt = FALSE; /// setup for no repeats

				// Send OVER

				EncodeAndSend4FSKControl(OVER, bytSessionID, LeaderLength);
				dttTimeoutTrip = Now;
				return;
			}

			// Try replying to IDLE with Data if something to send.

			// Can't transition straight to ISS as ISS may miss frame.
			// So stay in pending till AckByChar received.

			// ISS will send either AckByChar or another IDLE

			// ?? How do we handle missed AckByChar - old ISS won't repleat.
			// So treat same as sending BREAK and repeat. Try to set a different repeat
			// interval to reduce risk of repeated collisions

			if (blnFrameDecodedOK && intFrameType == IDLEFRAME) //  IF IDLE received from ISS 
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  IDLE received in ProtocolState %s substate %s", ARDOPStates[ProtocolState], ARQSubStates[ARQState]);
				{
					blnEnbARQRpt = FALSE; // setup for no repeats
				
					if (CheckForDisconnect())
						return;

	//				txSleep(250);		// Give chance to collect data from host

					if ((AutoBreak && bytDataToSendLength > 0) || blnBREAKCmd)
					{
						// keep BREAK Repeats fairly short (preliminary value 1 - 3 seconds)
		
						intFrameRepeatInterval = 2000; //ComputeInterFrameInterval(1000 + rand() % 2000);

						SetARDOPProtocolState(IRStoISS); // (ONLY IRS State where repeats are used)
						SendCommandToHost("STATUS QUEUE BREAK new Protocol State IRStoISS");
						blnEnbARQRpt = TRUE;  // setup for repeats until changeover
 //						EncodeAndSend4FSKControl(BREAK, bytSessionID, LeaderLength);
		
						WriteDebugLog(LOGDEBUG, "[ProcessRcvdARQFrame] IDLE with data to send. Send Data and enter IRStoISS");
						intLastARQDataFrameToHost = -1; // initialize to illegal value to capture first new ISS frame and pass to host

						if (bytCurrentFrameType == 0) //  hasn't been initialized yet
						{
							WriteDebugLog(LOGDEBUG, "[ProcessNewSamples, ProtocolState=IRStoISS, Initializing GetNextFrameData");
		   					GetNextFrameData(&intShiftUpDn, 0, "", TRUE); // just sets the initial data, frame type, and sets intShiftUpDn= 0
						}
	
						ResetTXState();
	
						SendData();				 //       Send new data from outbound queue and set up repeats
						return;
					}
					else
					{
						// Send ACK

						dttTimeoutTrip = Now;
						EncodeAndSend4FSKControl(ACK, bytSessionID, LeaderLength);
						dttTimeoutTrip = Now;
					}
					return;
				}
			}
            
			// handles DISCONNECT command from host
			
//			if (CheckForDisconnect())
//				return;

			// This handles normal data frames

			if (blnFrameDecodedOK && IsDataFrame(intFrameType)) // Frame even/odd toggling will prevent duplicates in case of missed ACK
			{
				if (intRmtLeaderMeas == 0)
				{
					intRmtLeaderMeas = intRmtLeaderMeasure;  // capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
					WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] IRS (receiving data) RmtLeaderMeas=%d ms", intRmtLeaderMeas);
				}

				if (ARQState == IRSData && blnBREAKCmd && intFrameType != bytLastACKedDataFrameType)
				{
					// if BREAK Command and first time the new frame type is seen then 
					// Handles Protocol Rule 3.4 
					// This means IRS wishes to BREAK so start BREAK repeats and go to protocol state IRStoISS 
					// This implements the  important IRS>ISS changeover...may have to adjust parameters here for reliability 
					// The incorporation of intFrameType <> objMain.bytLastACKedDataFrameType ensures only processing a BREAK on a frame
					// before it is ACKed to ensure the ISS will correctly capture the frame being sent in its purge buffer. 

					dttTimeoutTrip = Now;
					blnBREAKCmd = FALSE;
					blnEnbARQRpt = TRUE;  // setup for repeats until changeover
					intFrameRepeatInterval = ComputeInterFrameInterval(1000 + rand() % 2000);
					SetARDOPProtocolState(IRStoISS); // (ONLY IRS State where repeats are used)
					SendCommandToHost("STATUS QUEUE BREAK new Protocol State IRStoISS");
 					EncodeAndSend4FSKControl(BREAK, bytSessionID, LeaderLength);
					return;
				}

				if (intFrameType != intLastARQDataFrameToHost) // protects against duplicates if ISS missed IRS's ACK and repeated the same frame  
				{
					AddTagToDataAndSendToHost(bytData, "ARQ", DataLen); // only correct data in proper squence passed to host   
					intLastARQDataFrameToHost = intFrameType;
					dttTimeoutTrip = Now;
				}
				else
                    WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] Frame with same Type - Discard");
 				
				if (ARQState == IRSfromISS)
				{
					WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] Data Rcvd in ProtocolState=IRSData, Substate IRSfromISS Go to Substate IRSData");
					ARQState = IRSData;   //This substate change is the final completion of ISS to IRS changeover and allows the new IRS to now break if desired (Rule 3.5) 
				}
			
				// Always ACK good data frame ...ISS may have missed last ACK

				blnEnbARQRpt = FALSE;
				
//				if (intLastRcvdFrameQuality > intACKHiQThresholds[intFrameTypePtr])
//					EncodeAndSend4FSKControl(DataACKHiQ, bytSessionID, LeaderLength);
//				else
					EncodeAndSendMulticarrierACK(bytSessionID, LeaderLength);

				bytLastACKedDataFrameType = intFrameType;
				return;
			}

			// handles Data frame which did not decode correctly but was previously ACKed to ISS  Rev 0.4.3.1  2/28/2016  RM
			// this to handle marginal decoding cases where ISS missed an original ACK from IRS, IRS passed that data to host, and channel has 
			//  deteriorated to where data decode is now now not possible. 
 
			if ((!blnFrameDecodedOK) && intFrameType == bytLastACKedDataFrameType)
			{
				EncodeAndSendMulticarrierACK(bytSessionID, LeaderLength);
				blnEnbARQRpt = FALSE;

				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] Data Decode Failed but Frame Type matched last ACKed. Send ACK, data already passed to host. ");

				// handles Data frame which did not decode correctly (Failed CRC) and hasn't been acked before
			}
			else if ((!blnFrameDecodedOK) && IsDataFrame(intFrameType)) //Incorrectly decoded frame. Send NAK with Quality
			{
				if (ARQState == IRSfromISS)
				{
					WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] Data Frame Type Rcvd in ProtocolState=IRSData, Substate IRSfromISS Go to Substate IRSData");

					ARQState = IRSData;  //This substate change is the final completion of ISS to IRS changeover and allows the new IRS to now break if desired (Rule 3.5) 
				}
				blnEnbARQRpt = FALSE;
				
			//	if (intLastRcvdFrameQuality < intNAKLoQThresholds[intFrameTypePtr])
			//		EncodeAndSend4FSKControl(DataNAKLoQ, bytSessionID, LeaderLength);
			//	else
					EncodeAndSendMulticarrierACK(bytSessionID, LeaderLength);

				return;
			}
		}

		break;


		// IRStoISS State **************************************************************************************************
		// Must be just before ISS so we can drop through
	
		case IRStoISS: 
			
			// If we get an AckByCar we can switch to ISS
			
			if (blnFrameDecodedOK && intFrameType == AckByCar)
			{
				SetARDOPProtocolState(ISS);
				intLinkTurnovers += 1;
				ARQState = ISSData;

				// Drop through to process AckByCat
			}
			else
				break;



	// ISS state **************************************************************************************

	case ISS:
		
		if (ARQState == ISSConReq)  //' The ISS is sending Connect requests waiting for a ConAck from the remote IRS
		{
			// Session ID should be correct already (set by the ISS during first Con Req to IRS)
			// Process IRS Conack and capture IRS received leader for timing optimization
			// Process ConAck from IRS (Handles protocol rule 1.4)

			if (!blnFrameDecodedOK)
				return;

			if (intFrameType == ConAck)  // Process ConACK frames from IRS confirming BW is compatible and providing received leader info.
			{
				UCHAR bytDummy = 0;

				//WriteDebugLog(LOGDEBUG, ("[ARDOPprotocol.ProcessRcvdARQFrame] ISS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")

				intSessionBW = atoi(ARQBandwidths[ARQBandwidth]);
    			
//				CalculateOptimumLeader(10 * bytData[0], LeaderLength);
	
				// Initialize the frame type based on bandwidth
			
				GetNextFrameData(&intShiftUpDn, &bytDummy, NULL, TRUE);	// just sets the initial data frame type and sets intShiftUpDn = 0

				// prepare the ConACK answer with received leader length

				intReceivedLeaderLen = intLeaderRcvdMs;
				
				intFrameRepeatInterval = 2000;
				blnEnbARQRpt = TRUE;	// Setup for repeats of the ConACK if no answer from IRS
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] Compatible bandwidth received from IRS ConAck: %d Hz", intSessionBW);
				ARQState = ISSConAck;
				dttLastFECIDSent = Now;

				// V2 sends ID not ConAck

				EncLen = EncodePSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes, bytSessionID);
				Mod1Car50Bd4PSK(&bytEncodedBytes[0], EncLen, 0);		// only returns when all sent

				return;
			}
			
			if (intFrameType == ConRejBusy) // ConRejBusy Handles Protocol Rule 1.5
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBusy received from %s ABORT Connect Request", strRemoteCallsign);
 				sprintf(HostCmd, "REJECTEDBUSY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s, REMOTE STATION BUSY.", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				Abort();
				return;
			}
			if (intFrameType == ConRejBW) // ConRejBW Handles Protocol Rule 1.3
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBW received from %s ABORT Connect Request", strRemoteCallsign);
 				sprintf(HostCmd, "REJECTEDBW %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s, INCOMPATIBLE BW.", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				Abort();
				return;
			}
			return;		// Shouldn't get here

		}
		if (ARQState == ISSConAck)
		{
			if (blnFrameDecodedOK && (intFrameType == ACK || intFrameType == BREAK))  // if ACK received then IRS correctly received the ISS ConACK 
			{	
				// Note BREAK added per input from John W. to handle case where IRS has data to send and ISS missed the IRS's ACK from the ISS's ConACK Rev 0.5.3.1
				// Not sure about this. Not needed with AUTOBREAK but maybe with BREAK command

				if (intRmtLeaderMeas == 0)
				{
					intRmtLeaderMeas = intRmtLeaderMeasure; // capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
					WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] ISS RmtLeaderMeas=%d ms", intRmtLeaderMeas);
				}                       
				blnEnbARQRpt = FALSE;	// stop the repeats of ConAck and enables SendDataOrIDLE to get next IDLE or Data frame

				if (intFrameType == ACK && DebugLog)
					WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] ACK received in ARQState %s ", ARQSubStates[ARQState]);

				if (intFrameType == BREAK && DebugLog)
					WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] BREAK received in ARQState %s Processed as implied ACK", ARQSubStates[ARQState]);
                                        
				blnARQConnected = TRUE;
				bytLastARQDataFrameAcked = 1; // initialize to Odd value to start transmission frame on Even
				blnPending = FALSE;
   			
				sprintf(HostCmd, "CONNECTED %s %d", strRemoteCallsign, intSessionBW);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION ESTABLISHED WITH %s, SESSION BW = %d HZ", strRemoteCallsign, intSessionBW);
				QueueCommandToHost(HostCmd);

				ARQState = ISSData;

				intTrackingQuality = -1; //initialize tracking quality to illegal value
				intNAKctr = 0;
		
				if (intFrameType == BREAK && bytDataToSendLength == 0)
				{
					//' Initiate the transisiton to IRS
	
					WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] implied ACK, no data to send so action BREAK, send ACK");

					ClearDataToSend();
					blnEnbARQRpt = FALSE;  // setup for no repeats

					// Send ACK

					EncodeAndSend4FSKControl(OVER, bytSessionID, LeaderLength);
					dttTimeoutTrip = Now;
					SetARDOPProtocolState(IRS);
					ARQState = IRSfromISS;  // Substate IRSfromISS allows processing of Rule 3.5 later
	
					intLinkTurnovers += 1;
					intLastARQDataFrameToHost = -1;		// precondition to an illegal frame type (insures the new IRS does not reject a frame)
					memset(CarrierOk, 0, sizeof(CarrierOk));	// CLear MEM ARQ Stuff
					LastDataFrameType = intFrameType;
				}
				else
					SendData();				// Send new data from outbound queue and set up repeats
				
				return;
			}

			if (blnFrameDecodedOK && intFrameType == ConRejBusy)  // ConRejBusy
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBusy received in ARQState %s. Going to Protocol State DISC", ARQSubStates[ARQState]);

				sprintf(HostCmd, "REJECTEDBUSY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);

				SetARDOPProtocolState(DISC);
				InitializeConnection();
				return;
			}
			if (blnFrameDecodedOK && intFrameType == ConRejBW)	 // ConRejBW
			{
				//if (DebugLog) WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBW received in ARQState " & ARQState.ToString & " Going to Protocol State DISC")

				sprintf(HostCmd, "REJECTEDBW %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);

				SetARDOPProtocolState(DISC);
				InitializeConnection();
				return;
			}
			return;				// Shouldn't get here
		}
		
		if (ARQState == ISSData)
		{
			if (CheckForDisconnect())
				return;					// DISC sent
			
			if (!blnFrameDecodedOK)
				return;					// No decode so continue repeating either data or idle
                    
			// process ACK, NAK, DISC, END or BREAK here. Send ID if over 10 min. 
 
			if (intFrameType == ACK)	// if ACK
			{
				dttTimeoutTrip = Now;
				if (blnLastFrameSentData)
				{
#ifdef TEENSY
					SetLED(PKTLED, TRUE);		// Flash LED
					PKTLEDTimer = Now + 200;	// for 200 mS
#endif
					bytLastARQDataFrameAcked = bytLastARQDataFrameSent;
					
					if (bytQDataInProcessLen)
					{
						RemoveDataFromQueue(bytQDataInProcessLen);
						bytQDataInProcessLen = 0;
					}
					
					SendData();		 // Send new data from outbound queue and set up repeats
					return;
   				}
				return;
			}

			if (intFrameType == MultiACK)	// if ACK
			{
				dttTimeoutTrip = Now;
				SetLED(PKTLED, TRUE);		// Flash LED
				PKTLEDTimer = Now + 200;	// for 200 mS
				bytLastARQDataFrameAcked = bytLastARQDataFrameSent;		// Update toggle

				SendData();		 // Send new data from outbound queue and set up repeats
   			
				return;
			}

			
			if (intFrameType == BREAK)
			{
				if (!blnARQConnected)
				{
					// Handles the special case of this ISS missed last Ack from the 
					// IRS ConAck and remote station is now BREAKing to become ISS.
					// clean up the connection status
					
					blnARQConnected = TRUE;
					blnPending = FALSE;

					sprintf(HostCmd, "CONNECTED %s %d", strRemoteCallsign, intSessionBW);
					QueueCommandToHost(HostCmd);
					sprintf(HostCmd, "STATUS ARQ CONNECTION ESTABLISHED WITH %s, SESSION BW = %d HZ", strRemoteCallsign, intSessionBW);
					QueueCommandToHost(HostCmd);
				}
				
				//' Initiate the transisiton to IRS
		
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd from ARQState ISSData, Go to ProtocolState IRS & substate IRSfromISS , send ACK");

				// With new rules IRS can use BREAK to interrupt data from ISS. It will only
				// be sent on IDLE or changed data frame type, so we know the last sent data
				// wasn't processed by IRS

				if (bytDataToSendLength)
					SaveQueueOnBreak();			// Save the data so appl can restore it 

				ClearDataToSend();
				blnEnbARQRpt = FALSE;  // setup for no repeats
				intTrackingQuality = -1; // 'initialize tracking quality to illegal value
				intNAKctr = 0;
				SendCommandToHost("STATUS BREAK received from Protocol State ISS, new state IRS");

				// Send OVER

				EncodeAndSend4FSKControl(OVER, bytSessionID, LeaderLength);

				dttTimeoutTrip = Now;
				SetARDOPProtocolState(IRS);
				ARQState = IRSfromISS;  // Substate IRSfromISS allows processing of Rule 3.5 later
	
				intLinkTurnovers += 1;
				memset(CarrierOk, 0, sizeof(CarrierOk));	// CLear MEM ARQ Stuff
				LastDataFrameType = intFrameType;
				intLastARQDataFrameToHost = -1;		// precondition to an illegal frame type (insures the new IRS does not reject a frame)
				return;
			}

			if (intFrameType == DISCFRAME) // if DISC  Handles protocol rule 1.5
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received Send END...go to DISC state");
				if (AccumulateStats) LogStats();
					
				QueueCommandToHost("DISCONNECTED");
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
      
				bytLastARQSessionID = bytSessionID; // capture this session ID to allow answering DISC from DISC state
				blnDISCRepeating = FALSE;
				tmrFinalID = Now + 3000;
				ClearDataToSend();
				SetARDOPProtocolState(DISC);
				InitializeConnection();
				blnEnbARQRpt = FALSE;

				EncodeAndSend4FSKControl(END, bytSessionID, LeaderLength);
				return;
			}
				
			if (intFrameType == END)	// ' if END
			{
				WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.ProcessRcvdARQFrame]  END received ... going to DISC state");
				if (AccumulateStats) LogStats();
					
				QueueCommandToHost("DISCONNECTED");
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				ClearDataToSend();
				blnDISCRepeating = FALSE;

				if (CheckValidCallsignSyntax(strLocalCallsign))
				{
					dttLastFECIDSent = Now;
					EncLen = EncodePSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes, 0x3F);
					Mod1Car50Bd4PSK(&bytEncodedBytes[0], EncLen, 0);		// only returns when all sent
				}
					
				SetARDOPProtocolState(DISC);
				InitializeConnection();
				return;
			}
		}

	default:
		WriteDebugLog(LOGDEBUG, "Shouldnt get Here" );           
		//Logs.Exception("[ARDOPprotocol.ProcessRcvdARQFrame] 
	}
		// Unhandled Protocol state=" & GetARDOPProtocolState.ToString & "  ARQState=" & ARQState.ToString)
}


// Function to determine the IRS ConAck to reply based on intConReqFrameType received and local MCB.ARQBandwidth setting

int IRSNegotiateBW(int intConReqFrameType)
{
	//	returns the correct ConAck frame number to establish the session bandwidth to the ISS or the ConRejBW frame number if incompatible 
    //  if acceptable bandwidth sets stcConnection.intSessionBW

	switch (intConReqFrameType)
	{
	case ConReq200:

		if ((ARQBandwidth == XB200) || NegotiateBW)
		{
			intSessionBW = 200;
			return ConAck;
		}
		break;

	case ConReq500:

		if ((ARQBandwidth == XB500) || NegotiateBW && (ARQBandwidth == XB2500))
		{
			intSessionBW = 500;
			return ConAck;
		}
		break;

	case ConReq2500:
		
		if (ARQBandwidth == XB2500)
		{
			intSessionBW = 2500;
			return ConAck;
		}
		break;
	}

	return ConRejBW;
}

//	Function to send and ARQ connect request for the current MCB.ARQBandwidth
 
BOOL SendARQConnectRequest(char * strMycall, char * strTargetCall)
{
	// Psuedo Code:
	//  Determine the proper bandwidth and target call
	//  Go to the ISS State and ISSConREq sub state
	//  Encode the connect frame with extended Leader
	//  initialize the ConReqCount and set the Frame repeat interval
	//  (Handles protocol rule 1.1) 

	InitializeConnection();

	// Clear any queued data
	
	GetSemaphore();
	bytDataToSendLength = 0;
	FreeSemaphore();

	SetLED(TRAFFICLED, FALSE);

	GetNAKLoQLevels(atoi(ARQBandwidths[ARQBandwidth]));
	GetACKHiQLevels(atoi(ARQBandwidths[ARQBandwidth]));
  
	intRmtLeaderMeas = 0;
	strcpy(strRemoteCallsign, strTargetCall);
	strcpy(strLocalCallsign, strMycall);
	strcpy(strFinalIDCallsign, strLocalCallsign);

    bytSessionID = GenerateSessionID(strMycall, strTargetCall);
 
	EncLen = EncodeARQConRequest(strMycall, strTargetCall, ARQBandwidth, bytEncodedBytes);

	if (EncLen == 0)
		return FALSE;
	
	// generate the modulation with 2 x the default FEC leader length...Should insure reception at the target
	// Note this is sent with session ID 0x3F

	//	Set all flags before playing, as the End TX is called before we return here
	blnAbort = FALSE;
	dttTimeoutTrip = Now;
	SetARDOPProtocolState(ISS);
	ARQState = ISSConReq;    
	intRepeatCount = 1;

	displayCall('>', strTargetCall);
	
	bytSessionID = GenerateSessionID(strMycall, strTargetCall);  // Now set bytSessionID to receive ConAck (note the calling staton is the first entry in GenerateSessionID) 
	bytPendingSessionID = bytSessionID;

	WriteDebugLog(LOGINFO, "[SendARQConnectRequest] strMycall=%s  strTargetCall=%s bytPendingSessionID=%x", strMycall, strTargetCall, bytPendingSessionID);
	blnPending = TRUE;
	blnARQConnected = FALSE;
	
	intFrameRepeatInterval = 2000;  // ms ' Finn reported 7/4/2015 that 1600 was too short ...need further evaluation but temporarily moved to 2000 ms
	blnEnbARQRpt = TRUE;

	Mod1Car50Bd4PSK(&bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

	//' Update the main form menu status lable 
    //        Dim stcStatus As Status = Nothing
    //        stcStatus.ControlName = "mnuBusy"
    //        stcStatus.Text = "Calling " & strTargetCall
    //        queTNCStatus.Enqueue(stcStatus)

	return TRUE;
}


// Function to send 10 minute ID

BOOL Send10MinID()
{
	int dttSafetyBailout = 40;	// 100 mS intervals

	if (Now - dttLastFECIDSent > 600000 && !blnDISCRepeating)
	{

		// WriteDebugLog(LOGDEBUG, ("[ARDOPptocol.Send10MinID] Send ID Frame")
		// Send an ID frame (Handles protocol rule 4.0)

		blnEnbARQRpt = FALSE;

		dttLastFECIDSent = Now;
		EncLen = EncodePSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes, 0x3F);
		Mod1Car50Bd4PSK(&bytEncodedBytes[0], EncLen, 0);		// only returns when all sent		
		return TRUE;
	}
	return FALSE;
}

// Function to check for and initiate disconnect from a Host DISCONNECT command

BOOL CheckForDisconnect()
{
	if (blnARQDisconnect)
	{
		WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.CheckForDisconnect]  ARQ Disconnect ...Sending DISC (repeat)");
		
		QueueCommandToHost("STATUS INITIATING ARQ DISCONNECT");

		intFrameRepeatInterval = 2000;
		intRepeatCount = 1;
		blnARQDisconnect = FALSE;
		blnDISCRepeating = TRUE;
		blnEnbARQRpt = FALSE;

		// We could get here while sending an ACK (if host received a diconnect (bye) resuest
		// if so, don't send the DISC. ISS should go to Quiet, and we will repeat DISC

		if (SoundIsPlaying)
			return TRUE;

		EncodeAndSend4FSKControl(DISCFRAME, bytSessionID, LeaderLength);
		return TRUE;
	}
	return FALSE;
}

// subroutine to implement Host Command BREAK

void Break()
{
	time_t dttStartWait  = Now;

	if (ProtocolState != IRS)
	{
		WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.Break] |BREAK command received in ProtocolState: %s :", ARDOPStates[ProtocolState]);
		return;
	}
	
	WriteDebugLog(LOGDEBUG, "[ARDOPprotocol.Break] BREAK command received with AutoBreak = %d", AutoBreak);
	blnBREAKCmd = TRUE; // Set flag to process pending BREAK
}

// Function to abort an FEC or ARQ transmission 

void Abort()
{
	blnAbort = True;

	if (ProtocolState == IDLE || ProtocolState == IRS || ProtocolState == IRStoISS)
		GetNextARQFrame();
}

void ClearTuningStats()
{
	intLeaderDetects = 0;
	intLeaderSyncs = 0;
    intFrameSyncs = 0;
    intAccumFSKTracking = 0;
    intAccumPSKTracking = 0;
    intAccumQAMTracking = 0;
    intFSKSymbolCnt = 0;
    intPSKSymbolCnt = 0;
    intQAMSymbolCnt = 0;
    intGoodFSKFrameTypes = 0;
    intFailedFSKFrameTypes = 0;
    intGoodFSKFrameDataDecodes = 0;
    intFailedFSKFrameDataDecodes = 0;
    intGoodFSKSummationDecodes = 0;

    intGoodPSKFrameDataDecodes = 0;
    intGoodPSKSummationDecodes = 0;
    intFailedPSKFrameDataDecodes = 0;
    intGoodAPSKFrameDataDecodes = 0;
    intGoodAPSKSummationDecodes = 0;
    intFailedAPSKFrameDataDecodes = 0;
    intAvgFSKQuality = 0;
    intAvgPSKQuality = 0;
    dblFSKTuningSNAvg = 0;
    dblLeaderSNAvg = 0;
    dblAvgPSKRefErr = 0;
    intPSKTrackAttempts = 0;
    intQAMTrackAttempts = 0;
    dblAvgDecodeDistance = 0;
    intDecodeDistanceCount = 0;
    intShiftDNs = 0;
    intShiftUPs = 0;
    dttStartSession = Now;
	dttLastFECIDSent = Now;

    intLinkTurnovers = 0;
    intEnvelopeCors = 0;
    dblAvgCorMaxToMaxProduct = 0;
	intConReqSN = 0;
	intConReqQuality = 0;
	intTimeouts = 0;
}

void ClearQualityStats()
{
	intPSKQuality[0] = 0;
	intPSKQuality[1] = 0;
	intPSKQualityCnts[0] = 0;
	intPSKQualityCnts[1] = 0;	// Counts for 4PSK, 8PSK modulation modes 
    intPSKSymbolsDecoded = 0;

	intQAMQuality = 0;
	intQAMQualityCnts = 0;
    intQAMSymbolsDecoded = 0;

	BytesSent = BytesReceived = 0;
}

// Sub to Write Tuning Stats to the Debug Log 

extern int CarrierAcks;
extern int CarrierNaks;
extern int TotalCarriersSent;


void LogStats()
{
	int intTotPSKDecodes = intGoodPSKFrameDataDecodes + intFailedPSKFrameDataDecodes;
	int i;

	Statsprintf("************************* ARQ session stats with %s  %d minutes ****************************", strRemoteCallsign, (Now - dttStartSession) /60000); 
	Statsprintf("     LeaderDetects= %d   AvgLeader S+N:N(3KHz noise BW)= %f dB  LeaderSyncs= %d", intLeaderDetects, dblLeaderSNAvg - 23.8, intLeaderSyncs);
	Statsprintf("     AvgCorrelationMax:MaxProd= %f over %d  correlations", dblAvgCorMaxToMaxProduct, intEnvelopeCors);
	Statsprintf("     FrameSyncs=%d  Good Frame Type Decodes=%d  Failed Frame Type Decodes =%d Timeouts =%d", intFrameSyncs, intGoodFSKFrameTypes, intFailedFSKFrameTypes, intTimeouts);
	Statsprintf("     Avg Frame Type decode distance= %f over %d decodes", dblAvgDecodeDistance, intDecodeDistanceCount);
	Statsprintf(" ");
	Statsprintf("Bytes Sent %5d Bytes Received %5d", BytesSent,BytesReceived);


	if (intGoodPSKFrameDataDecodes + intFailedPSKFrameDataDecodes + intGoodPSKSummationDecodes > 0)
	{
		Statsprintf(" ");
		Statsprintf("  PSK:");
		Statsprintf("     Good PSK Data Frame Decodes=%d  RecoveredPSKCarriers with Summation=%d  Failed PSK Data Frame Decodes=%d", intGoodPSKFrameDataDecodes, intGoodPSKSummationDecodes, intFailedPSKFrameDataDecodes);
//		Statsprintf("     AccumPSKTracking=%d  %d attempts over %d total PSK Symbols",	intAccumPSKTracking, intPSKTrackAttempts, intPSKSymbolCnt);
	
		Statsprintf(" ");
	}
   
	if (intGoodAPSKFrameDataDecodes + intFailedAPSKFrameDataDecodes + intGoodAPSKSummationDecodes > 0)
	{
		Statsprintf(" ");
		Statsprintf("  APSK:");
		Statsprintf("     Good APSK Data Frame Decodes=%d  RecoveredAPSKCarriers with Summation=%d  Failed APSK Data Frame Decodes=%d", intGoodPSKFrameDataDecodes, intGoodPSKSummationDecodes, intFailedPSKFrameDataDecodes);
//		Statsprintf("     AccumPSKTracking=%d  %d attempts over %d total PSK Symbols",	intAccumPSKTracking, intPSKTrackAttempts, intPSKSymbolCnt);
	
		Statsprintf(" ");
	}

	Statsprintf("  Squelch= %d BusyDet= %d Mode Shift UPs= %d   Mode Shift DOWNs= %d  Link Turnovers= %d",
		Squelch, BusyDet, intShiftUPs, intShiftDNs, intLinkTurnovers);
	Statsprintf(" ");
	Statsprintf("  Received Frame Quality:");

	if (intPSKQualityCnts[0] > 0)
		Statsprintf("     Avg 4PSK Quality=%d on %d frame(s)",  intPSKQuality[0] / intPSKQualityCnts[0], intPSKQualityCnts[0]);

	if (intPSKQualityCnts[1] > 0)
		Statsprintf("     Avg 8PSK Quality=%d on %d frame(s)",  intPSKQuality[1] / intPSKQualityCnts[1], intPSKQualityCnts[1]);


	Statsprintf("");
	Statsprintf("Type               ACKS  NAKS CACKS CNAKS");

	for (i = 0; i < bytFrameTypesForBWLength; i++)
	{
		Statsprintf("%-17s %5d %5d %5d %5d", Name(bytFrameTypesForBW[i]),
			ModeHasWorked[i], ModeNAKS[i], CarrierACKS[i], CarrierNAKS[i]);
	}

	Statsprintf("Total Carriers Sent %5d Acked %5d Naked %5d", TotalCarriersSent, CarrierAcks, CarrierNaks);

	Statsprintf("************************************************************************************************");

	CloseStatsLog();
	CloseDebugLog();		// Flush debug log
}




















	