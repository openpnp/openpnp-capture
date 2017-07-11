/**

    Platform independent device information class.

*/

#ifndef deviceinfo_h
#define deviceinfo_h

#include <string>

/** device information struct/object */
class deviceInfo
{
public:
    virtual ~deviceInfo() {}

    std::string     m_name;         ///< UTF-8 printable name
};

#endif