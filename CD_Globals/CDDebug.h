#ifndef _CDDEBUG_H_
#define _CDDEBUG_H_

#include "c4d.h"

#include "CDQuaternion.h"
#include "CDCompatibility.h"


// debug check for messages
Bool ListReceivedCoreMessages(String str, LONG id, const BaseContainer &bc);
Bool ListReceivedMessages(String str, LONG type, void* data);

// debug console printing
String CDQuatToString(const CDQuaternion& q);
String VectorToString(const Vector& v);
String DescIDToString(const DescID& id);
String BCToString(const BaseContainer& bc);
String GeDataToString(const GeData& data);
String FourToString(LONG x);
String LongFourToString(LONG x);

// print redraw calls
void PrintRedraw(LONG type, String str);
void PrintCoffeeValueType(VALUE val);

#endif
