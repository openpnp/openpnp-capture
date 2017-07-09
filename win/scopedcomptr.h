/*

    Scoped COM pointer to auto-release the object
    when it goes out of scope

    Niels A. Moseley

*/

#ifndef scopedcomptr_h
#define scopedcomptr_h

template <typename Interface> class ScopedComPtr
{
public:
    ScopedComPtr(Interface* ptr) : m_ptr(ptr)
    {
        
    }

    ~ScopedComPtr()
    {
        if (m_ptr != nullptr)
        {
            InternalRelease();
        }
    }

    Interface* operator->() const
    {
        return m_ptr;
    }    

    /** replace the internal pointer with a new one,
        the old object is released */
    void Replace(Interface *newPtr)
    {
        InternalRelease();
        m_ptr = newPtr;
    }

    /** detach the object from the scoped pointer object
        without releasing it. */
    Interface* Detach()
    {
        Interface *obj = m_ptr;
        m_ptr = nullptr;
        return obj;
    }

private:
    void InternalRelease()
    {
        Interface * temp = m_ptr;
        if (temp != nullptr)
        {
            m_ptr = nullptr;
            temp->Release();
        }
    }

    //disallow copying of a scoped pointer!
    ScopedComPtr(ScopedComPtr const & other)
    {

    }

    Interface *m_ptr = nullptr;    
};

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#endif