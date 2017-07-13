/**

    Platform independent device information class.

*/

#ifndef deviceinfo_h
#define deviceinfo_h

#include <string>
#include <vector>
#include "openpnp-capture.h"

/** device information struct/object */
class deviceInfo
{
public:
    virtual ~deviceInfo() {}

    std::string                 m_name;     ///< UTF-8 printable name
    std::vector<CapFormatInfo>  m_formats;  ///< available buffer formats
};

#endif