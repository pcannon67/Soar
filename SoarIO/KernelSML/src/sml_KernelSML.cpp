/////////////////////////////////////////////////////////////////
// KernelSML class file.
//
// Author: Douglas Pearson, www.threepenny.net
// Date  : August 2004
//
// This class represents the translation point between SML and gSKI.
//
// It will maintain the information needed to communicate with gSKI
// and send and receive messages to the client (a tool or simulation).
//
/////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <crtdbg.h>
#endif

#include "sml_KernelSML.h"
#include "sml_Connection.h"
#include "sml_StringOps.h"

#include "gSKI.h"
#include <iostream>
#include <fstream>
#include <map>
#include <stdlib.h>

#include "IgSKI_KernelFactory.h"
#include "gSKI_Stub.h"
#include "IgSKI_Kernel.h"
#include "../../gSKI/src/gSKI_Error.h"
#include "gSKI_ErrorIds.h"
#include "gSKI_Enumerations.h"
#include "IgSKI_AgentManager.h"
#include "IgSKI_Agent.h"
#include "IgSKI_ProductionManager.h"
#include "IgSKI_OutputProcessor.h"
#include "IgSKI_InputProducer.h"
#include "IgSKI_Symbol.h"
#include "IgSKI_WME.h"

using namespace sml ;
using namespace gSKI ;

// BADBAD: Not sure where this macro is coming from but I saw this
// in IgSKI_Symbol.h and it's needed for the GetObject() calls to compile.
#ifdef _WIN32
#undef GetObject
#undef SendMessage
#endif

// Singleton instance of the kernel object
KernelSML* KernelSML::s_pKernel = NULL ;

static void DebugPrint(char const* pFilename, int line, char const* pMsg)
{
#ifdef _WIN32
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );

	_CrtDbgReport(_CRT_WARN, pFilename, line, "KernelSML", pMsg);
#endif
}

static ElementXML* AddErrorMsg(MessageGenerator* pConnection, ElementXML* pResponse, char const* pErrorMsg, int errorCode = -1)
{
	pConnection->AddErrorToSMLResponse(pResponse, pErrorMsg, errorCode) ;
	return pResponse ;
}

/*************************************************************
* @brief	Returns the singleton kernel object.
*************************************************************/
KernelSML* KernelSML::GetKernelSML()
{
	if (s_pKernel == NULL)
		s_pKernel = new KernelSML() ;

	return s_pKernel ;
}

KernelSML::KernelSML()
{
	// Create a Kernel Factory
	m_pKernelFactory = gSKI_CreateKernelFactory();
   
	// Create the kernel instance
	m_pIKernel = m_pKernelFactory->Create();

#ifdef USE_TCL_DEBUGGER
	m_Debugger = NULL ;
	m_DebuggerKernel = NULL ;
	m_DebuggerKernelFactory = NULL ;
#endif

	// Create the map from command name to handler function
	BuildCommandMap() ; 
}

KernelSML::~KernelSML()
{
	if (m_pKernelFactory && m_pIKernel)
		m_pKernelFactory->DestroyKernel(m_pIKernel);

}

/*************************************************************
* @brief	Return a string to the caller.
*
* @param pResult	This is the string to be returned.
* @returns	False if the string is NULL.
*************************************************************/
bool KernelSML::ReturnResult(Connection* pConnection, ElementXML* pResponse, char const* pResult)
{
	if (!pResult)
		return false ;

	pConnection->AddSimpleResultToSMLResponse(pResponse, pResult) ;

	return true ;
}

/*************************************************************
* @brief	Return an integer result to the caller.
*************************************************************/
bool KernelSML::ReturnIntResult(Connection* pConnection, ElementXML* pResponse, int result)
{
	char buffer[kMinBufferSize] ;
	Int2String(result, buffer, kMinBufferSize) ;

	pConnection->AddSimpleResultToSMLResponse(pResponse, buffer) ;

	return true ;
}

/*************************************************************
* @brief	Return a boolean result to the caller.
*************************************************************/
bool KernelSML::ReturnBoolResult(Connection* pConnection, ElementXML* pResponse, bool result)
{
	char const* pResult = result ? sml_Names::kTrue : sml_Names::kFalse ;
	pConnection->AddSimpleResultToSMLResponse(pResponse, pResult) ;
	return true ;
}

/*************************************************************
* @brief	Return an invalid argument error to the caller.
*************************************************************/
bool KernelSML::InvalidArg(Connection* pConnection, ElementXML* pResponse, char const* pCommandName, char const* pErrorDescription)
{
	std::string msg = "Invalid arguments for command : " ;
	msg += pCommandName ;
	msg += pErrorDescription ;

	AddErrorMsg(pConnection, pResponse, msg.c_str()) ;
	
	// Return true because we've already added the error message.
	return true ;
}

/*************************************************************
* @brief	Look up an agent from its name.
*************************************************************/
IAgent* KernelSML::GetAgent(char const* pAgentName)
{
	if (!pAgentName)
		return NULL ;

	IAgent* pAgent = GetKernel()->GetAgentManager()->GetAgent(pAgentName) ;
	return pAgent ;
}

/*************************************************************
* @brief	Take an incoming command and call the appropriate
*			handler to process it.
*************************************************************/
bool KernelSML::ProcessCommand(char const* pCommandName, Connection* pConnection, AnalyzeXML* pIncoming, ElementXML* pResponse)
{
	// Look up the function that handles this command
	CommandFunction pFunction = m_CommandMap[pCommandName] ;

	if (!pFunction)
	{
		// There is no handler for this command
		std::string msg = "Command " ;
		msg += pCommandName ;
		msg += " is not recognized by the kernel" ;

		AddErrorMsg(pConnection, pResponse, msg.c_str()) ;
		return false ;
	}

	// Look up the agent name parameter (most commands have this)
	char const* pAgentName = pIncoming->GetArgValue(sml_Names::kParamAgent) ;

	IAgent* pAgent = NULL ;

	if (pAgentName)
	{
		pAgent = GetAgent(pAgentName) ;

		if (!pAgent)
		{
			// Failed to find this agent
			std::string msg = "Could not find an agent with name: " ;
			msg += pAgentName ;
			AddErrorMsg(pConnection, pResponse, msg.c_str()) ;
			return false ;
		}
	}

	// Create a blank error code
	gSKI::Error error ;
	ClearError(&error) ;

	// Call to the handler (this is a pointer to member call so it's a bit odd)
	bool result = (this->*pFunction)(pAgent, pCommandName, pConnection, pIncoming, pResponse, &error) ;

	// If we return false, we report a generic error about the call.
	if (!result)
	{
		std::string msg = "The call " ;
		msg += pCommandName ;
		msg += " failed to execute correctly." ;
		if (isError(error))
		{
			msg += "gSKI error was: " ;
			msg += error.Text ;
			msg += " details: " ;
			msg += error.ExtendedMsg ;
		}

		AddErrorMsg(pConnection, pResponse, msg.c_str()) ;
		return false ;
	}

	return true ;
}

/*************************************************************
* @brief	Takes an incoming SML message and responds with
*			an appropriate response message.
*
* @param pConnection	The connection this message came in on.
* @param pIncoming		The incoming message
*************************************************************/
ElementXML* KernelSML::ProcessIncomingSML(Connection* pConnection, ElementXML* pIncomingMsg)
{
	if (!pIncomingMsg || !pConnection)
		return NULL ;

#ifdef DEBUG
	// For debugging, it's helpful to be able to look at the incoming message as an XML string
	char* pIncomingXML = pIncomingMsg->GenerateXMLString(true) ;
#endif

	ElementXML* pResponse = pConnection->CreateSMLResponse(pIncomingMsg) ;

	// Fatal error creating the response
	if (!pResponse)
		return NULL ;

	// Analyze the message and find important tags
	AnalyzeXML msg ;
	msg.Analyze(pIncomingMsg) ;

	// Get the "name" attribute from the <command> tag
	char const* pCommandName = msg.GetCommandName() ;

	if (pCommandName)
	{
		ProcessCommand(pCommandName, pConnection, &msg, pResponse) ;
	}
	else
	{
		// The message wasn't something we recognize.
		if (!msg.GetCommandTag())
			AddErrorMsg(pConnection, pResponse, "Incoming message did not contain a <command> tag") ;
		else
			AddErrorMsg(pConnection, pResponse, "Incoming message did not contain a name attribute in the <command> tag") ;
	}

#ifdef DEBUG
	// For debugging, it's helpful to be able to look at the response as XML
	char* pResponseXML = pResponse->GenerateXMLString(true) ;

	// Set a break point on this next line if you wish to see the incoming
	// and outgoing as XML before they get deleted.
	ElementXML::DeleteString(pIncomingXML) ;
	ElementXML::DeleteString(pResponseXML) ;
#endif

	return pResponse ;
}

/*************************************************************
* @brief	Converts an id from a client side value to a kernel side value.
*			We need to be able to do this because the client is adding a collection
*			of wmes at once, so it makes up the ids for those objects.
*			But the kernel will assign them a different value when the
*			wme is actually added in the kernel.
*************************************************************/
bool KernelSML::ConvertID(char const* pClientID, std::string* pKernelID)
{
	if (pClientID == NULL)
		return false ;

	IdentifierMapIter iter = m_IdentifierMap.find(pClientID) ;

	if (iter == m_IdentifierMap.end())
	{
		// If the client id is not in the map, then we may have been
		// passed a kernel id (this will happen at times).
		// So return the value we were passed
		*pKernelID = pClientID ;
		return false ;
	}
	else
	{
		// If we found a mapping, return the mapped value
		*pKernelID = iter->second ;
		return true ;
	}
}

void KernelSML::RecordIDMapping(char const* pClientID, char const* pKernelID)
{
	m_IdentifierMap[pClientID] = pKernelID ;
}

/*************************************************************
* @brief	Converts a time tag from a client side value to
*			a kernel side one.
*************************************************************/
long KernelSML::ConvertTimeTag(char const* pTimeTag)
{
	if (pTimeTag == NULL)
		return -1 ;

	TimeTagMapIter iter = m_TimeTagMap.find(pTimeTag) ;

	if (iter == m_TimeTagMap.end())
	{
		return -1 ;
	}
	else
	{
		// If we found a mapping, return the mapped value
		long result = iter->second ;
		return result ;
	}
}

/*************************************************************
* @brief	Converts a time tag from a client side value to
*			a kernel side one.
*************************************************************/
void KernelSML::RecordTimeTag(char const* pTimeTag, long kernelTimeTag)
{
	m_TimeTagMap[pTimeTag] = kernelTimeTag ;
}

