#include <iostream>
#include <string>


////// Define the type of interface to Soar that you're using:
//#define GSKI_DIRECT
//#define SML_THROUGH_GSKI
//#define SML_SGIO_HYBRID
//#define SGIO_DIRECT

#include "AgnosticTowers.h"

#ifdef SGIO_DIRECT
	#include "sgioTowers.h"
#endif

//#include "gSKITowersSoarAgent.h"


//TgDI directives
//#include "TgD.h"
//#include "tcl.h"

//TgDI directives
#ifdef _WIN32
	#define _WINSOCKAPI_
	#include <Windows.h>
	#define TGD_SLEEP Sleep
#else
	#include <unistd.h>
	#define TGD_SLEEP usleep
#endif


#ifdef GSKI_DIRECT
	//gSKI directives
	#include "IgSKI_InputProducer.h"
	#include "IgSKI_OutputProcessor.h"
	#include "IgSKI_InputLink.h"
	#include "IgSKI_OutputLink.h"
	//#include "IgSKI_WorkingMemory.h"
	#include "gSKI_Structures.h"

	//command line interface directives
	#include "cli_CommandLineInterface.h"

	using namespace gSKI;
	using namespace cli;
#endif
	
#ifdef SML_SGIO_HYBRID
	//SML Directives
	#include "sml_Client.h"
	using namespace sml;
#endif

#ifdef SML_THROUGH_GSKI
	//SML Directives
	#include "sml_Client.h"
	using namespace sml;
#endif

using std::cout; using std::cin; using std::string;
using std::endl;


const int defaultNumTowers = 3;
const int defaultNumdisks = 11;

int main(int argc, char* argv[])
{
	bool doPrinting = true;
	int numTowers = defaultNumTowers;
	int numdisks = defaultNumdisks;

	if(argc > 1)
	{
		if(!strcmp(argv[1], "false"))
			doPrinting = false;
		// @TODO more checking, for robustness 
	}

	if(argc > 2)
	{
		numTowers = atoi(argv[3]);
		if(numTowers < 3)
			numTowers = 3;
	}

	//It would be flexible to read in the number of disks, but the productions are hard-coded to 11
	//if(argc > 3)
	//{
	//	numdisks = atoi(argv[3]);
	//	if(numdisks < 5)
	//		numdisks = 5;

	//}



/*  FIXME TODO this will need to be added for any interface that uses sml
#ifdef SML_THROUGH_GSKI
	//create connection for SML version
	ErrorCode error;
	Connection* pConnection = Connection::CreateEmbeddedConnection("KernelSML", &error);
	IKernelFactory* kFactory = sml_CreateKernelFactory(pConnection);
#endif*/
/*

*/
#ifdef USE_GSKI_DIRECT_NOT_SML
	CommandLineInterface* commLine = new CommandLineInterface();
	commLine->SetKernel(kernel);
	char* pResponse = "\0";//we don't need to allocate space for this
	gSKI::Error* pError = new Error();
	assert(commLine->DoCommand(agent, "pushd ../examples/towers", pResponse, pError));
	assert(commLine->DoCommand(agent, "source towers-of-hanoi-86.soar", pResponse, pError));
	assert(commLine->DoCommand(agent, "popd", pResponse, pError));
#endif

	//=============================================================================
	//========================= Setup the TgDI for the TSI ========================
	// Determine tsi version to use via command line argument
	//TgD::TSI_VERSION tsiVersion;
	/*const char * usage = "[ 25 | 40 ]";
	if (argc > 2)
	{
		// "TgDITestd" should be argv[0] but windows puts the entire path in there
		std::cout << "Towers: too many arguments.\nUsage: towers " << usage << std::endl;
		return 1;
	}
	else if (argc == 2)
	{
		if (strcmp(argv[1], "25") == 0)
		{
			tsiVersion = TgD::TSI25;
		}
		else if (strcmp(argv[1], "40") == 0)
		{
			tsiVersion = TgD::TSI40;
		}
		else
		{ 
			// "TgDITestd" should be argv[0] but windows puts the entire path in there
			std::cout << "towers: incorrect argument.\nUsage: towers " << usage << std::endl;
			return 1;
		}
	}
	else*/
	/*{
		std::cout << "No TSI version specified, defaulting to 4.0.0" << std::endl; 
		tsiVersion = TgD::TSI40;
	}*/


	//create debugger
	//TgD::TgD* debugger = CreateTgD(agent, kernel, kFactory->GetKernelVersion(), tsiVersion, argv[0]);
	//debugger->Init();

	//=============================================================================
	//=============================================================================
//	IInputLink* iLink = agent->GetInputLink();

	{
		if(doPrinting)
			cout << "***Welcome to Towers of Hanoi***" << endl << endl;

		HanoiWorld hanoi(doPrinting, numTowers);

//		SoarAgent soarAgent(agent, &hanoi);

		//register the SoarAgent as the output processor
//		IOutputLink* oLink = agent->GetOutputLink();
		//oLink->AddOutputProcessor("move-disk", &soarAgent);

		if(doPrinting)
			hanoi.Print();

		while(!hanoi.AtGoalState())
		{
			hanoi.Run();
//			soarAgent.MakeMove();

			if(doPrinting)
				hanoi.Print();
		}
//		soarAgent.MakeMove();//this allows the agent to catch up to the game world
		//after the goal state
	}

//	while(TgD::TgD::Update(false, debugger))
//		TGD_SLEEP(50);

	// Wait for the user to press return to exit the program. (So window doesn't just vanish).
	printf("\n\nPress <non-whitespace char> then enter to exit\n") ;
	string garbage;
	cin>>garbage;
	return 0;
}

