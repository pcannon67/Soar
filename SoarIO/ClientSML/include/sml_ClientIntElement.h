/////////////////////////////////////////////////////////////////
// IntElement class
//
// Author: Douglas Pearson, www.threepenny.net
// Date  : Sept 2004
//
// Working memory element that has an int value.
//
/////////////////////////////////////////////////////////////////

#ifndef SML_INT_ELEMENT_H
#define SML_INT_ELEMENT_H

#include "sml_ClientWMElement.h"

#include <string>

namespace sml {

class WorkingMemory ;
class Identifier ;

class IntElement : public WMElement
{
	// Allow working memory to create these objects directly (user must use agent class to do this)
	friend WorkingMemory ;

protected:
	// The value for this wme is an int
	int		m_Value ;

	// We need to convert to a string form at times
	std::string m_StringForm ;

public:
	// Returns the type of the value stored here (e.g. "string" or "int" etc.)
	virtual char const* GetValueType()	;

	// Returns a string form of the value stored here.
	virtual char const* GetValueAsString() ;

	int GetValue() { return m_Value ; }

protected:
	IntElement(Agent* pAgent, Identifier* pID, char const* pAttributeName, int value) ;
	virtual ~IntElement(void);

	void SetValue(int value)
	{
		m_Value = value ;
	}
};

}	// namespace

#endif // SML_INT_ELEMENT_H
